#include "app.h"
#include "style.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

namespace {
    struct Point {
        std::string name;
        float coords[3];
        bool hidden = false;;
        float color[3] = {1.0f, 0.5f, 0.0f}; // Default orange color
    };

    struct Line {
        std::string name;
        int point1index;
        int point2index;
        float color[3] = {1.0f, 1.0f, 1.0f}; // Default white color
    };

    struct Plane { // planes: you will be able to choose its opacity
        std::string name;
        int point1index;
        int point2index;
        int point3index;
        float color[3] = {0.0f, 1.0f, 0.0f}; 
    };

    struct Settings {
        float backgroundColor[3] = {0.0f, 0.0f, 0.0f};

        float dihedralBackgroundColor[3] = {0.1f, 0.1f, 0.1f};
        float dihedralLineColor[3] = {1.0f, 1.0f, 1.0f};

        int axesType = 1; // Default to Cartesian axes

        bool showDihedralSystem = true;
        bool showCutPoints = true;
        bool showCutLines = true;

        float mouseSensitivity = 0.2f;
        float cameraDistance = 5.5f;

        float pointSize = 8.0f;
        float lineThickness = 3.0f;
    };

    static double m_scrollY;

    std::vector<Point> s_points;
    std::vector<Line> s_lines;
    Settings s_settings;
}

App::App() : m_window(nullptr), m_isMousePressed(false), 
             m_lastMouseX(0), m_lastMouseY(0), m_jsonHandler("./assets/presets.json") {}

App::~App() = default;

bool App::Initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    m_window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Sistema Diedrico", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    // Set resize callback
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
        app->m_windowWidth = width;
        app->m_windowHeight = height;
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwMakeContextCurrent(m_window);

    #ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    #endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetCustomStyle();

    // font
    ImGuiIO& io = ImGui::GetIO();
    font = io.Fonts->AddFontFromFileTTF("./assets/Roboto-Regular.ttf", 14);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    if (!m_renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    // Load JSON file
    m_jsonHandler = JsonHandler("./assets/presets.json");

    if (!m_jsonHandler.LoadJson()) {
        std::cerr << "Failed to load JSON file\n";
        m_jsonLoaded = false;
    } else {
        m_jsonLoaded = true;
    }
    
    return true;
}

void DrawSettingsWindow(Renderer& renderer, Camera& camera) {
    ImGui::Begin("3D Representation Settings");

    ImGui::ColorEdit3("Background Color", s_settings.backgroundColor);
    ImGui::ColorEdit3("Dihedral Bg Color", s_settings.dihedralBackgroundColor);
    ImGui::ColorEdit3("Dihedral Line Color", s_settings.dihedralLineColor);

    const char* axesTypes[] = {"3D Axes", "Cartesian Axes", "Dihedral Axes ONLY"};
    ImGui::Combo("Axes Type", &s_settings.axesType, axesTypes, IM_ARRAYSIZE(axesTypes));
    renderer.SetAxesType(s_settings.axesType);

    ImGui::Checkbox("Show Dihedral System", &s_settings.showDihedralSystem);
    renderer.SetDihedralsVisible(s_settings.showDihedralSystem);

    //ImGui::SliderFloat("Camera Distance", &s_settings.cameraDistance, 1.0f, 20.0f);
    //camera.SetDistance(s_settings.cameraDistance);
    
    ImGui::SliderFloat("Mouse Sensitivity", &s_settings.mouseSensitivity, 0.0f, 2.0f);
    camera.SetSensitivity(s_settings.mouseSensitivity);

    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", 
                camera.GetPosition().x, 
                camera.GetPosition().y, 
                camera.GetPosition().z);

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}

void DrawMenuBar(GLFWwindow* window) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Save*", nullptr, false);
            ImGui::MenuItem("Save As*", nullptr, false);
            ImGui::MenuItem("Load*", nullptr, false);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("App")) {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("About")) {
            ImGui::Text("Version 0.3");
            ImGui::Text("Made by Alonso Martínez");
            ImGui::Text("@almartdev on GitHub");
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void DrawTabsWindow(Renderer& renderer) {
    ImGui::Begin("Properties");

    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Points"))  // Points -----------
        {  
            static char pointName[128] = "";
            static float pointCoords[3] = {0.0f, 0.0f, 0.0f};

            ImGui::InputText("Name", pointName, sizeof(pointName));
            ImGui::InputFloat3("Coordinates", pointCoords);

            if (ImGui::Button("Add Point")) {
                std::string name = pointName;
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

                if (name.empty()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                } 
                else if (std::any_of(s_points.begin(), s_points.end(), 
                                    [&](const Point& p) { return p.name == name; })) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
                } 
                else {
                    s_points.push_back({name, {pointCoords[0], pointCoords[1], pointCoords[2]}});
                    pointName[0] = '\0';
                    memset(pointCoords, 0, sizeof(pointCoords));
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Show Cut Points", &s_settings.showCutPoints);
            renderer.SetCutPointVisible(s_settings.showCutPoints);

            ImGui::DragFloat("Point Size", &s_settings.pointSize, 0.1f, 0.1f, 100.0f);

            ImGui::Separator();
            
            // Point list
            for (size_t i = 0; i < s_points.size(); ++i) {
                if (s_points[i].hidden) continue;

                auto& point = s_points[i];
                ImVec4 color(point.color[0], point.color[1], point.color[2], 1.0f);

                ImGui::Text("%s (%c)", point.name.c_str(), point.name[0]);
                ImGui::SameLine();

                ImGui::PushID(static_cast<int>(i));
                ImGui::DragFloat3("", point.coords, 0.1f);
                ImGui::SameLine();

                ImGui::ColorEdit4("##Color", (float*)&color, 
                                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                point.color[0] = color.x;
                point.color[1] = color.y;
                point.color[2] = color.z;

                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    s_points.erase(s_points.begin() + i);
                    --i;
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lines")) // Lines -------------
        { 
            ImGui::Text("Select two points to create a line");

            static char lineName[128] = "";
            ImGui::InputText("Name", lineName, sizeof(lineName));

            // dropdown to select two points
            static int selectedPoint1 = -1;
            static int selectedPoint2 = -1;

            ImGui::Combo("Point 1", &selectedPoint1, 
                        [](void* data, int idx, const char** out_text) {
                            auto& points = *static_cast<std::vector<Point>*>(data);
                            if (idx < 0 || idx >= static_cast<int>(points.size())) {
                                return false;
                            }
                            *out_text = points[idx].name.c_str();
                            return true;
                        }, &s_points, s_points.size(), ImGuiComboFlags_HeightSmall);
            ImGui::Combo("Point 2", &selectedPoint2, 
                        [](void* data, int idx, const char** out_text) {
                            auto& points = *static_cast<std::vector<Point>*>(data);
                            if (idx < 0 || idx >= static_cast<int>(points.size())) {
                                return false;
                            }
                            *out_text = points[idx].name.c_str();
                            return true;
                        }, &s_points, s_points.size(), ImGuiComboFlags_NoArrowButton);

            // thickness slider
            ImGui::DragFloat("Line Thickness", &s_settings.lineThickness, 0.1f, 0.1f, 100.0f);
            
            if (ImGui::Button("Add Line")) {
                if (selectedPoint1 != selectedPoint2) {

                    std::string name = lineName;
                    name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                    name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));
            
                    if (name.empty()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                    } 
                    else if (std::any_of(s_points.begin(), s_points.end(), 
                                        [&](const Point& p) { return p.name == name; })) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
                    } 
                    // check for points with the same coords (same point really)
                    else if (s_points[selectedPoint1].coords == s_points[selectedPoint2].coords) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Points are the same");
                    } 
                    else { // add line!
                        s_lines.push_back({lineName, 
                                        selectedPoint1, 
                                        selectedPoint2});  
                                        
                        lineName[0] = '\0';
                    }
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Show Cut Lines", &s_settings.showCutLines);
            renderer.SetCutLineVisible(s_settings.showCutLines);

            ImGui::Separator();
            
            // Line list
            for (size_t i = 0; i < s_lines.size(); ++i) {
                auto& line = s_lines[i];
                ImVec4 color(line.color[0], line.color[1], line.color[2], 1.0f);

                ImGui::Text("%s (%c): ", line.name.c_str(), line.name[0]);
                ImGui::SameLine();
                ImGui::Text("%s --> %s", s_points[line.point1index].name.c_str(), s_points[line.point2index].name.c_str());

                ImGui::SameLine();
                ImGui::PushID(static_cast<int>(i));
                ImGui::ColorEdit4("##Color", (float*)&color, 
                                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                line.color[0] = color.x;
                line.color[1] = color.y;
                line.color[2] = color.z;

                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    s_lines.erase(s_lines.begin() + i);
                    --i;
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Planes")) {
            // DrawPlanesWindow(renderer);
            ImGui::Text("Not yet!");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DrawPresetWindow(JsonHandler& jsonHandler, bool& jsonLoaded) {
    ImGui::Begin("Presets", nullptr);
    ImGui::Text("Select a preset to load");

    if (!jsonLoaded) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load presets!");
        ImGui::End();
        return;
    }

    // Draw preset buttons
    if (ImGui::Button("Points")) {
        ImGui::OpenPopup("Points Presets");
    }
    
    if (ImGui::BeginPopupModal("Points Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Point Presets:");
        ImGui::Separator();
        
        auto pointPresets = jsonHandler.GetPointPresets();
        if (pointPresets.empty()) {
            ImGui::Text("No point presets found");
        } else {
            for (const auto& preset : pointPresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    // Add the point to the scene
                    s_points.push_back({
                        preset["name"],
                        {
                            preset["coords"]["d"].get<float>(),
                            preset["coords"]["a"].get<float>(),
                            preset["coords"]["c"].get<float>()
                        }
                    });
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Lines")) {
        ImGui::OpenPopup("Lines Presets");
    }
    
    if (ImGui::BeginPopupModal("Lines Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Line Presets:");
        ImGui::Separator();
        
        auto linePresets = jsonHandler.GetLinePresets();
        if (linePresets.empty()) {
            ImGui::Text("No line presets found");
        } else {
            for (const auto& preset : linePresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    // Add the line to the scene
                    s_lines.push_back({
                        preset["name"],
                        static_cast<int>(s_points.size()), // Index of first point
                        static_cast<int>(s_points.size() + 1) // Index of second point
                    });
                    // Add the two points for the line
                    // TODO: make the points apear in ui as a group tied to the line, if you delete the line, delete the points too and vice versa
                    // Right now they are just hidden.
                    s_points.push_back({
                        ("A_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point1"]["d"].get<float>(),
                            preset["point1"]["a"].get<float>(),
                            preset["point1"]["c"].get<float>()
                        },
                        true // hidden
                    });
                    s_points.push_back({
                        ("B_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point2"]["d"].get<float>(),
                            preset["point2"]["a"].get<float>(),
                            preset["point2"]["c"].get<float>()
                        },
                        true 
                    });
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Planes")) {
        ImGui::OpenPopup("Planes Presets");
    }
    
    if (ImGui::BeginPopupModal("Planes Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Plane presets coming soon!");
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void App::DrawDihedralViewport() { // TODO: this whole function should be maybe on renderer class or even on a new class
    // Push custom style for this window only
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); // Square corners
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(s_settings.dihedralBackgroundColor[0], s_settings.dihedralBackgroundColor[1], s_settings.dihedralBackgroundColor[2], 1.0f)); // Dark background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Light border
        
    ImGui::Begin("Dihedral Projection", nullptr, 
                ImGuiWindowFlags_NoCollapse | 
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoTitleBar );

    // default color
    ImColor s_color = IM_COL32(s_settings.dihedralLineColor[0] * 255, 
                        s_settings.dihedralLineColor[1] * 255, 
                        s_settings.dihedralLineColor[2] * 255, 255);

    // Get the size of the window
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    // Ground line (L.T.) //
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 p0 = ImVec2(cursorPos.x, cursorPos.y + viewportSize.y / 2);
    ImVec2 p1 = ImVec2(cursorPos.x + viewportSize.x, cursorPos.y + viewportSize.y / 2);
    drawList->AddLine(p0, p1, s_color, 2.0f); 

    // draw the two little lines under each side of the ground line that explain that that is the ground line
    drawList->AddLine(ImVec2(p0.x + 4, p0.y + 5), ImVec2(p0.x + 25, p0.y + 5), s_color, 2.0f);
    drawList->AddLine(ImVec2(p1.x - 4, p1.y + 5), ImVec2(p1.x - 25, p1.y + 5), s_color, 2.0f);

    // Origin point
    ImVec2 p2 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 - 5);
    ImVec2 p3 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 + 5);
    drawList->AddLine(p2, p3, s_color, 2.0f); // L.T. line
    
    // Draw points in 2D projection
    for (const auto& point : s_points) {
        if (point.hidden) continue;

        float x = point.coords[0] / 2.0f; // careful for the scaling here, this should not be hardcoded! (same with the 3D view)
        float y1 = point.coords[2] / 3.0f;
        float y2 = -point.coords[1] / 3.0f;

        // Consider Im calling any point P1 or P2 for a point called "P"

        // no scaling for now, we coma back to this later

        // fist point representation (P1)  
        ImVec2 pos1(cursorPos.x + viewportSize.x / 2 + x * 10, (cursorPos.y + viewportSize.y / 2) - y1 * 10);

        ImVec2 pos2(cursorPos.x + viewportSize.x / 2 + x * 10, (cursorPos.y + viewportSize.y / 2) - y2 * 10);

        drawList->AddCircleFilled(pos1, s_settings.pointSize / 2,  // this is P2 (vertical plane)
                                IM_COL32(point.color[0] * 255, point.color[1] * 255, point.color[2] * 255, 255));
        drawList->AddCircleFilled(pos2, s_settings.pointSize / 2,  // this is P1 (horizontal plane)
                                IM_COL32(point.color[0] * 255, point.color[1] * 255, point.color[2] * 255, 255));
        
        // Draw labels (we only use the first character)
        ImGui::SetCursorScreenPos(ImVec2(pos2.x - 30, pos2.y - 20)); // P1
        ImGui::TextColored(s_color, "%c1", point.name[0]);
        ImGui::SetCursorScreenPos(ImVec2(pos1.x - 30, pos1.y - 20)); // P2
        ImGui::TextColored(s_color, "%c2", point.name[0]); 

        // Draw projection lines
        drawList->AddLine(pos1, pos2, s_color, 1.0f);
    }
    
    // Draw lines between points
    for (const auto& line : s_lines) {
        float x1 = s_points[line.point1index].coords[0] / 2.0f;
        float y1 = s_points[line.point1index].coords[2] / 3.0f;
        float y2 = -s_points[line.point1index].coords[1] / 3.0f;

        float x2 = s_points[line.point2index].coords[0] / 2.0f;
        float y3 = s_points[line.point2index].coords[2] / 3.0f;
        float y4 = -s_points[line.point2index].coords[1] / 3.0f;

        // r2 line (vertical plane)
        ImVec2 p1_r2(cursorPos.x + viewportSize.x / 2 + x1 * 10, (cursorPos.y + viewportSize.y / 2) - y1 * 10);
        ImVec2 p2_r2(cursorPos.x + viewportSize.x / 2 + x2 * 10, (cursorPos.y + viewportSize.y / 2) - y3 * 10);
        
        float m_r2 = (p2_r2.y - p1_r2.y) / (p2_r2.x - p1_r2.x);
        float b_r2 = p1_r2.y - m_r2 * p1_r2.x;
        
        float r2_minX = cursorPos.x;
        float r2_maxX = cursorPos.x + viewportSize.x;
        float r2_minY = cursorPos.y;
        float r2_maxY = cursorPos.y + viewportSize.y;
        
        ImVec2 edge1_r2, edge2_r2;
        
        if (abs(p2_r2.x - p1_r2.x) > abs(p2_r2.y - p1_r2.y)) {
            edge1_r2 = ImVec2(r2_minX, m_r2 * r2_minX + b_r2);
            edge2_r2 = ImVec2(r2_maxX, m_r2 * r2_maxX + b_r2);
        } else {
            edge1_r2 = ImVec2((r2_minY - b_r2) / m_r2, r2_minY);
            edge2_r2 = ImVec2((r2_maxY - b_r2) / m_r2, r2_maxY);
        }

        // r1 line (horizontal plane)
        ImVec2 p1_r1(cursorPos.x + viewportSize.x / 2 + x1 * 10, (cursorPos.y + viewportSize.y / 2) - y2 * 10);
        ImVec2 p2_r1(cursorPos.x + viewportSize.x / 2 + x2 * 10, (cursorPos.y + viewportSize.y / 2) - y4 * 10);
        
        float m_r1 = (p2_r1.y - p1_r1.y) / (p2_r1.x - p1_r1.x);
        float b_r1 = p1_r1.y - m_r1 * p1_r1.x;
        
        float r1_minX = cursorPos.x;
        float r1_maxX = cursorPos.x + viewportSize.x;
        float r1_minY = cursorPos.y;
        float r1_maxY = cursorPos.y + viewportSize.y;
        
        ImVec2 edge1_r1, edge2_r1;
        
        if (abs(p2_r1.x - p1_r1.x) > abs(p2_r1.y - p1_r1.y)) {
            edge1_r1 = ImVec2(r1_minX, m_r1 * r1_minX + b_r1);
            edge2_r1 = ImVec2(r1_maxX, m_r1 * r1_maxX + b_r1);
        } else {
            edge1_r1 = ImVec2((r1_minY - b_r1) / m_r1, r1_minY);
            edge2_r1 = ImVec2((r1_maxY - b_r1) / m_r1, r1_maxY);
        }

        // TODO: user selects the color of the line also for the dihedral representation
        drawList->AddLine(edge1_r2, edge2_r2, s_color, 1.0f);
        drawList->AddLine(edge1_r1, edge2_r1, s_color, 1.0f);

        // draw labels on the middle of the lines
        ImVec4 s_textColor = ImVec4(s_settings.dihedralLineColor[0], 
                            s_settings.dihedralLineColor[1], 
                            s_settings.dihedralLineColor[2], 1.0f);
        ImGui::SetCursorScreenPos(ImVec2((edge1_r1.x + edge2_r1.x) / 2, (edge1_r1.y + edge2_r1.y) / 2 - 25)); // r1
        ImGui::TextColored(s_textColor, "%c1", line.name[0]);
        ImGui::SetCursorScreenPos(ImVec2((edge1_r2.x + edge2_r2.x) / 2, (edge1_r2.y + edge2_r2.y) / 2 - 25)); // r2
        ImGui::TextColored(s_textColor, "%c2", line.name[0]);
    }

    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void App::Run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        
        // Get current window size
        glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Handle mouse input
        double mouseX, mouseY;
        glfwGetCursorPos(m_window, &mouseX, &mouseY);

        if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            if (!m_isMousePressed) {
                m_isMousePressed = true;
                m_lastMouseX = static_cast<float>(mouseX);
                m_lastMouseY = static_cast<float>(mouseY);
            }
            
            float deltaX = static_cast<float>(mouseX) - m_lastMouseX;
            float deltaY = static_cast<float>(mouseY) - m_lastMouseY;
            
            m_camera.ProcessMouseMovement(deltaX, deltaY);
            
            m_lastMouseX = static_cast<float>(mouseX);
            m_lastMouseY = static_cast<float>(mouseY);
        } else {
            m_isMousePressed = false;
        }

        auto scrollCallback = [](GLFWwindow* window, double xoffset, double yoffset) {
            m_scrollY = yoffset;
        };

        glfwSetScrollCallback(m_window, scrollCallback);

        if (m_scrollY != 0) {
            m_camera.SetDistance(m_camera.GetDistance() - static_cast<float>(m_scrollY) * .3f);
            m_scrollY = 0.0; // Reset scroll value after processing
        }

        // close on esc key, not big fan but we'll keep it for now
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(m_window, true);
        }
        
        // Clear screen
        glClearColor(s_settings.backgroundColor[0], 
                    s_settings.backgroundColor[1], 
                    s_settings.backgroundColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set viewport to center of the window
        int viewportWidth = m_windowWidth;
        int viewportHeight = m_windowHeight;
        
        // fixed aspect ratio:
        // int size = std::min(m_windowWidth, m_windowHeight);
        // viewportWidth = size;
        // viewportHeight = size;
        
        int viewportX = (m_windowWidth - viewportWidth) / 2;
        int viewportY = (m_windowHeight - viewportHeight) / 2;
        
        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

        // Draw UI
        DrawMenuBar(m_window);
        DrawSettingsWindow(m_renderer, m_camera);

        DrawPresetWindow(m_jsonHandler, m_jsonLoaded);

        // TODO: refactor this because the renderer argument shouldnt be needed.
        DrawTabsWindow(m_renderer);
        
        DrawDihedralViewport(); // Draw the new 2D viewport

        // Prepare point data
        std::vector<glm::vec3> pointPositions, pointColors;
        for (const auto& point : s_points) {
            if (point.hidden) continue; // Skip hidden points
            pointPositions.emplace_back(point.coords[0]/50, point.coords[2]/50, point.coords[1]/50);
            pointColors.emplace_back(point.color[0], point.color[1], point.color[2]);
        }

        // Prepare line data
        std::vector<std::pair<glm::vec3, glm::vec3>> linePositions;
        std::vector<glm::vec3> lineColors;
        for (const auto& line : s_lines) {
            const auto& point1 = s_points[line.point1index];
            const auto& point2 = s_points[line.point2index];

            linePositions.emplace_back(glm::vec3(point1.coords[0]/50, point1.coords[2]/50, point1.coords[1]/50),
                                       glm::vec3(point2.coords[0]/50, point2.coords[2]/50, point2.coords[1]/50));
            lineColors.emplace_back(line.color[0], line.color[1], line.color[2]);
        }

        // Render scene
        m_renderer.Render();

        m_renderer.DrawPoints(pointPositions, pointColors, s_settings.pointSize);
        m_renderer.DrawLines(linePositions, lineColors, s_settings.lineThickness);

        m_renderer.UpdateCamera(m_camera, viewportWidth, viewportHeight);
        
        // Render ImGui and swap buffers
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

void App::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}