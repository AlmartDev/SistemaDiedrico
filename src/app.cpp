#include "app.h"
#include "style.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

#include "scene.h"

double App::m_scrollY = 0.0;

#ifndef __EMSCRIPTEN__
    App::App() : m_window(nullptr), m_jsonHandler("./assets/presets.json") {}
#else
    App::App() : m_window(nullptr), m_jsonHandler(nullptr) {}
#endif

bool App::Initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // Add these hints specifically for Emscripten
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Sistema Diedrico", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
        app->m_windowWidth = width;
        app->m_windowHeight = height;
    });

    glfwMakeContextCurrent(m_window);

#ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
#endif

    printf("GLFW initialized: %d\n", glfwInit());
    printf("Window created: %p\n", m_window);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SetupImGui();

    if (!m_renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    m_jsonLoaded = m_jsonHandler.LoadJson();
    if (!m_jsonLoaded) {
        std::cerr << "Failed to load JSON file\n";
    }

    return true;
}

void App::SetupImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetCustomStyle();

    ImGuiIO& io = ImGui::GetIO();
    
#ifndef __EMSCRIPTEN__
    m_font = io.Fonts->AddFontFromFileTTF("./assets/Roboto-Regular.ttf", m_sceneData.settings.fontSize);
    // load imgui.ini
    ImGui::GetIO().IniFilename = "./assets/imgui.ini";
#endif

    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 100");
#endif
}

void App::HandleInput() {
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

    //if (m_scrollY != 0) {
    //    m_camera.SetDistance(m_camera.GetDistance() - static_cast<float>(m_scrollY) * .3f);
    //    m_scrollY = 0.0;
    //}

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
}

void App::PrepareRenderData() {
    std::vector<glm::vec3> pointPositions, pointColors;
    for (const auto& point : m_sceneData.points) {
        if (point.hidden) continue;
        pointPositions.emplace_back(point.coords[0]/50, point.coords[2]/50, point.coords[1]/50);
        pointColors.emplace_back(point.color[0], point.color[1], point.color[2]);
    }

    std::vector<std::pair<glm::vec3, glm::vec3>> linePositions;
    std::vector<glm::vec3> lineColors;
    for (const auto& line : m_sceneData.lines) {
        const auto& point1 = m_sceneData.points[line.point1index];
        const auto& point2 = m_sceneData.points[line.point2index];

        linePositions.emplace_back(
            glm::vec3(point1.coords[0]/50, point1.coords[2]/50, point1.coords[1]/50),
            glm::vec3(point2.coords[0]/50, point2.coords[2]/50, point2.coords[1]/50)
        );
        lineColors.emplace_back(line.color[0], line.color[1], line.color[2]);
    }

    std::vector<std::vector<glm::vec3>> planePositions;
    std::vector<glm::vec3> planeColors;
    std::vector<bool> planeExpand;

    for (const auto& plane : m_sceneData.planes) {
        const auto& point1 = m_sceneData.points[plane.point1index];
        const auto& point2 = m_sceneData.points[plane.point2index];
        const auto& point3 = m_sceneData.points[plane.point3index];

        planePositions.push_back({
            glm::vec3(point1.coords[0]/50, point1.coords[2]/50, point1.coords[1]/50),
            glm::vec3(point2.coords[0]/50, point2.coords[2]/50, point2.coords[1]/50),
            glm::vec3(point3.coords[0]/50, point3.coords[2]/50, point3.coords[1]/50)
        });
        planeColors.emplace_back(plane.color[0], plane.color[1], plane.color[2]);

        planeExpand.push_back(plane.expand);
    }

    m_renderer.DrawPoints(pointPositions, pointColors, m_sceneData.settings.pointSize);
    m_renderer.DrawLines(linePositions, lineColors, m_sceneData.settings.lineThickness);
    m_renderer.DrawPlanes(planePositions, planeColors, planeExpand, m_sceneData.settings.planeOpacity);
}

void App::DrawUI() {
    DrawMenuBar();
    DrawSettingsWindow();
    DrawPresetWindow();
    DrawTabsWindow();
    DrawDihedralViewport();
}

void App::DrawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Save*", nullptr, false);
            ImGui::MenuItem("Save As*", nullptr, false);
            ImGui::MenuItem("Load*", nullptr, false);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("App")) {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(m_window, true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("About")) {
            ImGui::Text("Version 0.5");
            ImGui::Text("Made by Alonso Martínez");
            ImGui::Text("@almartdev on GitHub");
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void App::DrawSettingsWindow() {
    ImGui::Begin("3D Representation Settings");

    ImGui::ColorEdit3("Background Color", m_sceneData.settings.backgroundColor);
    ImGui::ColorEdit3("Dihedral Bg Color", m_sceneData.settings.dihedralBackgroundColor);
    ImGui::ColorEdit3("Dihedral Line Color", m_sceneData.settings.dihedralLineColor);

    const char* axesTypes[] = {"3D Axes", "Cartesian Axes", "Dihedral Axes ONLY"};
    ImGui::Combo("Axes Type", &m_sceneData.settings.axesType, axesTypes, IM_ARRAYSIZE(axesTypes));
    m_renderer.SetAxesType(m_sceneData.settings.axesType);

    if (m_sceneData.settings.axesType != 2) {
        ImGui::Checkbox("Show Dihedral System", &m_sceneData.settings.showDihedralSystem);
        m_renderer.SetDihedralsVisible(m_sceneData.settings.showDihedralSystem);
    }
    
    ImGui::SliderFloat("Mouse Sensitivity", &m_sceneData.settings.mouseSensitivity, 0.0f, 2.0f);
    m_camera.SetSensitivity(m_sceneData.settings.mouseSensitivity); 
    
    ImGui::SliderFloat("Camera Distance", &m_sceneData.settings.cameraDistance, 0.1f, 25.0f);
    m_camera.SetDistance(m_sceneData.settings.cameraDistance);

    /*ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", 
                m_camera.GetPosition().x, 
                m_camera.GetPosition().y, 
                m_camera.GetPosition().z);*/

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}

void App::DrawTabsWindow() {
    ImGui::Begin("Properties");

    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Points")) {
            DrawPointsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lines")) {
            DrawLinesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Planes")) {
            DrawPlanesTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void App::DrawPointsTab() {
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
        else if (std::any_of(m_sceneData.points.begin(), m_sceneData.points.end(), 
                            [&](const SceneData::Point& p) { return p.name == name; })) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
        } 
        else {
            m_sceneData.points.push_back({name, {pointCoords[0], pointCoords[1], pointCoords[2]}});
            pointName[0] = '\0';
            memset(pointCoords, 0, sizeof(pointCoords));
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show Cut Points", &m_sceneData.settings.showCutPoints);
    m_renderer.SetCutPointVisible(m_sceneData.settings.showCutPoints);

    ImGui::DragFloat("Point Size", &m_sceneData.settings.pointSize, 0.1f, 0.1f, 100.0f);

    ImGui::Separator();
    
    // Draw point list with better styling
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    if (ImGui::BeginTable("PointTable", 4, ImGuiTableFlags_NoHostExtendX                
                                         | ImGuiTableFlags_RowBg  
                                         | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn(" Name");
        ImGui::TableSetupColumn("Coords");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < m_sceneData.points.size(); ++i) {
            if (m_sceneData.points[i].hidden) continue;

            auto& point = m_sceneData.points[i];
            ImVec4 color(point.color[0], point.color[1], point.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s (%c)", point.name.c_str(), point.name.empty() ? '?' : point.name[0]);

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(static_cast<int>(i));
            // Make the DragFloat3 as wide as possible in the cell
            float cellWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(cellWidth);
            ImGui::DragFloat3("", point.coords, 0.1f);
            ImGui::PopID();

            ImGui::TableSetColumnIndex(2);
            if (ImGui::ColorEdit4("##Color", (float*)&color, 
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                point.color[0] = color.x;
                point.color[1] = color.y;
                point.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(3);
            if (ImGui::Button("X")) {
                DeletePoint(point);
            }
            
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

void App::DrawLinesTab() {
    ImGui::Text("Select two points to create a line");

    static char lineName[128] = "";
    ImGui::InputText("Name", lineName, sizeof(lineName));

    static int selectedPoint1 = -1;
    static int selectedPoint2 = -1;

    // filtered list of visible points
    std::vector<int> visiblePointIndices;
    std::vector<const char*> visiblePointNames;
    for (size_t i = 0; i < m_sceneData.points.size(); ++i) {
        if (!m_sceneData.points[i].hidden) {
            visiblePointIndices.push_back(static_cast<int>(i));
            visiblePointNames.push_back(m_sceneData.points[i].name.c_str());
        }
    }

    auto getOriginalIndex = [&](int selected) -> int {
        if (selected >= 0 && selected < static_cast<int>(visiblePointIndices.size()))
            return visiblePointIndices[selected];
        return -1;
    };

    int selectedIdx1 = -1, selectedIdx2 = -1;
    for (size_t i = 0; i < visiblePointIndices.size(); ++i) {
        if (visiblePointIndices[i] == selectedPoint1) selectedIdx1 = static_cast<int>(i);
        if (visiblePointIndices[i] == selectedPoint2) selectedIdx2 = static_cast<int>(i);
    }

    ImGui::Combo("Point 1", &selectedIdx1, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
    ImGui::Combo("Point 2", &selectedIdx2, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));

    selectedPoint1 = getOriginalIndex(selectedIdx1);
    selectedPoint2 = getOriginalIndex(selectedIdx2);

    ImGui::DragFloat("Line Thickness", &m_sceneData.settings.lineThickness, 0.1f, 0.1f, 100.0f);

    if (ImGui::Button("Add Line")) {
        if (selectedPoint1 != selectedPoint2) {
            std::string name = lineName;
            name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
            name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

            if (name.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
            } else if (std::any_of(m_sceneData.points.begin(), m_sceneData.points.end(),
                [&](const SceneData::Point& p) { return p.name == name; })) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
            } else if (m_sceneData.points[selectedPoint1].coords == m_sceneData.points[selectedPoint2].coords) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Points are the same");
            } else {
                m_sceneData.lines.push_back({ lineName, selectedPoint1, selectedPoint2 });
                lineName[0] = '\0';
            }
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show Cut Lines", &m_sceneData.settings.showCutLines);
    m_renderer.SetCutLineVisible(m_sceneData.settings.showCutLines);

    ImGui::Separator();

    // Draw line list with better styling
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    if (ImGui::BeginTable("LineTable", 4, ImGuiTableFlags_NoHostExtendX                
                                        | ImGuiTableFlags_RowBg  
                                        | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Points");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < m_sceneData.lines.size(); ++i) {
            auto& line = m_sceneData.lines[i];
            ImVec4 color(line.color[0], line.color[1], line.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s (%c)", line.name.c_str(), line.name.empty() ? '?' : line.name[0]);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s --> %s",
                m_sceneData.points[line.point1index].name.c_str(),
                m_sceneData.points[line.point2index].name.c_str());

            /*ImGui::TableSetColumnIndex(2);
            bool pointsHidden = m_sceneData.points[line.point1index].hidden && m_sceneData.settings.showCutPoints;
            if (ImGui::Checkbox("##Hide", &pointsHidden)) {
                m_sceneData.points[line.point1index].hidden = pointsHidden;
                m_sceneData.points[line.point2index].hidden = pointsHidden;
            }*/

            ImGui::TableSetColumnIndex(2);
            if (ImGui::ColorEdit4("##Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                line.color[0] = color.x;
                line.color[1] = color.y;
                line.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(3);
            if (ImGui::Button("X")) {
                m_sceneData.lines.erase(m_sceneData.lines.begin() + i);
                DeletePoint(m_sceneData.points[line.point1index]);
                DeletePoint(m_sceneData.points[line.point2index]);
                ImGui::PopID();
                --i; // Re-adjust index
                continue;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar(); 
}

void App::DrawPlanesTab() {
    static char planeName[128] = "";
    ImGui::InputText("Name", planeName, sizeof(planeName));

    ImGui::DragFloat("Plane Opacity", &m_sceneData.settings.planeOpacity, 0.01f, 0.1f, 1.0f);

    // tabs to creat point with coords or select existing points
    if (ImGui::BeginTabBar("PlaneTabs")) {
        if (ImGui::BeginTabItem("Select Points")) {
            static int selectedPoint1 = -1;
            static int selectedPoint2 = -1;
            static int selectedPoint3 = -1;

            // filtered list of visible points
            std::vector<int> visiblePointIndices;
            std::vector<const char*> visiblePointNames;
            for (size_t i = 0; i < m_sceneData.points.size(); ++i) {
                if (!m_sceneData.points[i].hidden) {
                    visiblePointIndices.push_back(static_cast<int>(i));
                    visiblePointNames.push_back(m_sceneData.points[i].name.c_str());
                }
            }

            auto getOriginalIndex = [&](int selected) -> int {
                if (selected >= 0 && selected < static_cast<int>(visiblePointIndices.size()))
                    return visiblePointIndices[selected];
                return -1;
            };

            int selectedIdx1 = -1, selectedIdx2 = -1, selectedIdx3 = -1;
            for (size_t i = 0; i < visiblePointIndices.size(); ++i) {
                if (visiblePointIndices[i] == selectedPoint1) selectedIdx1 = static_cast<int>(i);
                if (visiblePointIndices[i] == selectedPoint2) selectedIdx2 = static_cast<int>(i);
                if (visiblePointIndices[i] == selectedPoint3) selectedIdx3 = static_cast<int>(i);
            }

            ImGui::Combo("Point 1", &selectedIdx1, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
            ImGui::Combo("Point 2", &selectedIdx2, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
            ImGui::Combo("Point 3", &selectedIdx3, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));

            selectedPoint1 = getOriginalIndex(selectedIdx1);
            selectedPoint2 = getOriginalIndex(selectedIdx2);
            selectedPoint3 = getOriginalIndex(selectedIdx3);

            if (ImGui::Button("Add Plane")) {
                if (selectedPoint1 != selectedPoint2 && selectedPoint1 != selectedPoint3 && selectedPoint2 != selectedPoint3) {
                    std::string name = planeName;
                    name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                    name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

                    if (name.empty()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                    } else if (std::any_of(m_sceneData.points.begin(), m_sceneData.points.end(),
                        [&](const SceneData::Point& p) { return p.name == name; })) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
                    } else {
                        m_sceneData.planes.push_back({ planeName, selectedPoint1, selectedPoint2, selectedPoint3 });
                        planeName[0] = '\0';
                    }
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Add Coords")) {
            static float planeCoords[3] = {0.0f, 0.0f, 0.0f};
            ImGui::InputFloat3("Coords", planeCoords);

            if (ImGui::Button("Add Plane")) {
                std::string name = planeName;
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

                if (name.empty()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                } else if (std::any_of(m_sceneData.points.begin(), m_sceneData.points.end(),
                    [&](const SceneData::Point& p) { return p.name == name; })) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
                } else {
                    // we add the 3 points
                    m_sceneData.points.push_back({ name + "1", { planeCoords[0], 0.0f, 0.0f }, true });
                    m_sceneData.points.push_back({ name + "2", { 0.0f, planeCoords[1], 0.0f }, true });
                    m_sceneData.points.push_back({ name + "3", { 0.0f, 0.0f, planeCoords[2] }, true });

                    m_sceneData.planes.push_back({ name, static_cast<int>(m_sceneData.points.size() - 3), 
                                                    static_cast<int>(m_sceneData.points.size() - 2), 
                                                    static_cast<int>(m_sceneData.points.size() - 1) });
                    planeName[0] = '\0';
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    //ImGui::Checkbox("Show Cuts*", &m_sceneData.settings.showCutPlanes);
    //m_renderer.SetCutPlaneVisible(m_sceneData.settings.showCutPlanes);

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    if (ImGui::BeginTable("PlaneTable", 6, ImGuiTableFlags_NoHostExtendX                
                                        | ImGuiTableFlags_RowBg  
                                        | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Points");
        ImGui::TableSetupColumn("Hide Points");
        ImGui::TableSetupColumn("Expand");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < m_sceneData.planes.size(); ++i) {
            auto& plane = m_sceneData.planes[i];
            ImVec4 color(plane.color[0], plane.color[1], plane.color[2], 1.0f);
            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s (%c)", plane.name.c_str(), plane.name.empty() ? '?' : plane.name[0]);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s, %s, %s",
                m_sceneData.points[plane.point1index].name.c_str(),
                m_sceneData.points[plane.point2index].name.c_str(),
                m_sceneData.points[plane.point3index].name.c_str());

            ImGui::TableSetColumnIndex(2);
            bool pointsHidden = m_sceneData.points[plane.point1index].hidden && 
                                m_sceneData.points[plane.point2index].hidden && 
                                m_sceneData.points[plane.point3index].hidden && 
                                m_sceneData.settings.showCutPoints;

            if (ImGui::Checkbox("##Hide", &pointsHidden)) {
                m_sceneData.points[plane.point1index].hidden = pointsHidden;
                m_sceneData.points[plane.point2index].hidden = pointsHidden;
                m_sceneData.points[plane.point3index].hidden = pointsHidden;
            }

            ImGui::TableSetColumnIndex(3);
            if (ImGui::Checkbox("##Expand", &plane.expand)) {
                m_sceneData.planes[i].expand = plane.expand; 
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::ColorEdit4("##Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                plane.color[0] = color.x;
                plane.color[1] = color.y;
                plane.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(5);
            if (ImGui::Button("X")) {
                m_sceneData.planes.erase(m_sceneData.planes.begin() + i);
                DeletePoint(m_sceneData.points[plane.point1index]);
                DeletePoint(m_sceneData.points[plane.point2index]);
                DeletePoint(m_sceneData.points[plane.point3index]);
                ImGui::PopID();
                --i; // Re-adjust index
                continue;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar(); 
}

void App::DrawPresetWindow() {
    ImGui::Begin("Presets", nullptr);
    ImGui::Text("Select a preset to load");

    if (!m_jsonLoaded) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load presets!");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Points")) {
        ImGui::OpenPopup("Points Presets");
    }
    
    if (ImGui::BeginPopupModal("Points Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Point Presets:");
        ImGui::Separator();
        
        auto pointPresets = m_jsonHandler.GetPointPresets();
        if (pointPresets.empty()) {
            ImGui::Text("No point presets found");
        } else {
            for (const auto& preset : pointPresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    m_sceneData.points.push_back({
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
        
        auto linePresets = m_jsonHandler.GetLinePresets();
        if (linePresets.empty()) {
            ImGui::Text("No line presets found");
        } else {
            for (const auto& preset : linePresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    m_sceneData.lines.push_back({
                        preset["name"],
                        static_cast<int>(m_sceneData.points.size()),
                        static_cast<int>(m_sceneData.points.size() + 1)
                    });
                    
                    m_sceneData.points.push_back({
                        ("A_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point1"]["d"].get<float>(),
                            preset["point1"]["a"].get<float>(),
                            preset["point1"]["c"].get<float>()
                        },
                        true
                    });
                    m_sceneData.points.push_back({
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
        ImGui::OpenPopup("Plane Presets");
    }
    
    if (ImGui::BeginPopupModal("Plane Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Plane Presets:");
        ImGui::Separator();
        
        auto planePresets = m_jsonHandler.GetPlanePresets();
        if (planePresets.empty()) {
            ImGui::Text("No plane presets found");
        } else {
            for (const auto& preset : planePresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    m_sceneData.planes.push_back({
                        preset["name"],
                        static_cast<int>(m_sceneData.points.size()),
                        static_cast<int>(m_sceneData.points.size() + 1),
                        static_cast<int>(m_sceneData.points.size() + 2)
                    });
                    
                    m_sceneData.points.push_back({
                        ("A_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point1"]["d"].get<float>(),
                            preset["point1"]["a"].get<float>(),
                            preset["point1"]["c"].get<float>()
                        },
                        true // hidden
                    });
                    m_sceneData.points.push_back({
                        ("B_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point2"]["d"].get<float>(),
                            preset["point2"]["a"].get<float>(),
                            preset["point2"]["c"].get<float>()
                        },
                        true 
                    });
                    m_sceneData.points.push_back({
                        ("C_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point3"]["d"].get<float>(),
                            preset["point3"]["a"].get<float>(),
                            preset["point3"]["c"].get<float>()
                        },
                        true 
                    });

                    m_sceneData.planes.back().expand = preset["expand"].get<bool>();

                    // check if there is a specific color
                    if (preset.contains("color")) {
                        m_sceneData.planes.back().color[0] = preset["color"]["r"].get<float>();
                        m_sceneData.planes.back().color[1] = preset["color"]["g"].get<float>();
                        m_sceneData.planes.back().color[2] = preset["color"]["b"].get<float>();
                    }

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

    ImGui::End();
}

void App::DeletePoint(SceneData::Point& point) { // this isnt good
    point.hidden = true;
    point.name = "deleted";
}

void App::DrawDihedralViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(
        m_sceneData.settings.dihedralBackgroundColor[0], 
        m_sceneData.settings.dihedralBackgroundColor[1], 
        m_sceneData.settings.dihedralBackgroundColor[2], 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        
    ImGui::Begin("Dihedral Projection", nullptr, 
                ImGuiWindowFlags_NoCollapse | 
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoTitleBar);

    ImColor lineColor = IM_COL32(
        m_sceneData.settings.dihedralLineColor[0] * 255, 
        m_sceneData.settings.dihedralLineColor[1] * 255, 
        m_sceneData.settings.dihedralLineColor[2] * 255, 255);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Ground line (L.T.)
    ImVec2 p0 = ImVec2(cursorPos.x, cursorPos.y + viewportSize.y / 2);
    ImVec2 p1 = ImVec2(cursorPos.x + viewportSize.x, cursorPos.y + viewportSize.y / 2);
    drawList->AddLine(p0, p1, lineColor, 2.0f); 

    // Small indicator lines
    drawList->AddLine(ImVec2(p0.x + 4, p0.y + 5), ImVec2(p0.x + 25, p0.y + 5), lineColor, 2.0f);
    drawList->AddLine(ImVec2(p1.x - 4, p1.y + 5), ImVec2(p1.x - 25, p1.y + 5), lineColor, 2.0f);

    // Origin point
    ImVec2 p2 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 - 5);
    ImVec2 p3 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 + 5);
    drawList->AddLine(p2, p3, lineColor, 2.0f);
    
    // Draw points in 2D projection
    for (const auto& point : m_sceneData.points) {
        if (point.hidden) continue;

        float x = point.coords[0] / 2.0f;
        float y1 = point.coords[2] / 3.0f;
        float y2 = -point.coords[1] / 3.0f;

        ImVec2 pos1(cursorPos.x + viewportSize.x / 2 + x * 10, (cursorPos.y + viewportSize.y / 2) - y1 * 10);
        ImVec2 pos2(cursorPos.x + viewportSize.x / 2 + x * 10, (cursorPos.y + viewportSize.y / 2) - y2 * 10);

        drawList->AddCircleFilled(pos1, m_sceneData.settings.pointSize / 2,
                                IM_COL32(point.color[0] * 255, point.color[1] * 255, point.color[2] * 255, 255));
        drawList->AddCircleFilled(pos2, m_sceneData.settings.pointSize / 2,
                                IM_COL32(point.color[0] * 255, point.color[1] * 255, point.color[2] * 255, 255));
        
        // Draw labels
        ImGui::SetCursorScreenPos(ImVec2(pos2.x - 20, pos2.y - 20));
        ImGui::Text("%c1", point.name[0]);
        ImGui::SetCursorScreenPos(ImVec2(pos1.x - 20, pos1.y - 20));
        ImGui::Text("%c2", point.name[0]);

        // Draw projection lines
        drawList->AddLine(pos1, pos2, lineColor, 1.0f);
    }
    
    // lines
    for (const auto& line : m_sceneData.lines) {
        const auto& p1 = m_sceneData.points[line.point1index];
        const auto& p2 = m_sceneData.points[line.point2index];

        float x1 = p1.coords[0] / 2.0f;
        float y1 = p1.coords[2] / 3.0f;
        float y2 = -p1.coords[1] / 3.0f;

        float x2 = p2.coords[0] / 2.0f;
        float y3 = p2.coords[2] / 3.0f;
        float y4 = -p2.coords[1] / 3.0f;

        // r2 line (vertical plane)
        ImVec2 p1_r2(cursorPos.x + viewportSize.x / 2 + x1 * 10, (cursorPos.y + viewportSize.y / 2) - y1 * 10);
        ImVec2 p2_r2(cursorPos.x + viewportSize.x / 2 + x2 * 10, (cursorPos.y + viewportSize.y / 2) - y3 * 10);
        
        float r2_minX = cursorPos.x;
        float r2_maxX = cursorPos.x + viewportSize.x;
        float r2_minY = cursorPos.y;
        float r2_maxY = cursorPos.y + viewportSize.y;
        
        ImVec2 edge1_r2, edge2_r2;
        
        // Handle vertical lines (x1 == x2)
        if (abs(p2_r2.x - p1_r2.x) < 0.0001f) {
            edge1_r2 = ImVec2(p1_r2.x, r2_minY);
            edge2_r2 = ImVec2(p1_r2.x, r2_maxY);
        }
        // Handle horizontal lines (y1 == y2)
        else if (abs(p2_r2.y - p1_r2.y) < 0.0001f) {
            edge1_r2 = ImVec2(r2_minX, p1_r2.y);
            edge2_r2 = ImVec2(r2_maxX, p1_r2.y);
        }
        else {
            float m_r2 = (p2_r2.y - p1_r2.y) / (p2_r2.x - p1_r2.x);
            float b_r2 = p1_r2.y - m_r2 * p1_r2.x;
            
            if (abs(p2_r2.x - p1_r2.x) > abs(p2_r2.y - p1_r2.y)) {
                edge1_r2 = ImVec2(r2_minX, m_r2 * r2_minX + b_r2);
                edge2_r2 = ImVec2(r2_maxX, m_r2 * r2_maxX + b_r2);
            } else {
                edge1_r2 = ImVec2((r2_minY - b_r2) / m_r2, r2_minY);
                edge2_r2 = ImVec2((r2_maxY - b_r2) / m_r2, r2_maxY);
            }
        }
        
        drawList->AddLine(edge1_r2, edge2_r2, lineColor, 1.0f);

        // r1 line (horizontal plane)
        ImVec2 p1_r1(cursorPos.x + viewportSize.x / 2 + x1 * 10, (cursorPos.y + viewportSize.y / 2) - y2 * 10);
        ImVec2 p2_r1(cursorPos.x + viewportSize.x / 2 + x2 * 10, (cursorPos.y + viewportSize.y / 2) - y4 * 10);
        
        float r1_minX = cursorPos.x;
        float r1_maxX = cursorPos.x + viewportSize.x;
        float r1_minY = cursorPos.y;
        float r1_maxY = cursorPos.y + viewportSize.y;
        
        ImVec2 edge1_r1, edge2_r1;
        
        // Handle vertical lines (x1 == x2)
        if (abs(p2_r1.x - p1_r1.x) < 0.0001f) {
            edge1_r1 = ImVec2(p1_r1.x, r1_minY);
            edge2_r1 = ImVec2(p1_r1.x, r1_maxY);
        }
        // Handle horizontal lines (y1 == y2)
        else if (abs(p2_r1.y - p1_r1.y) < 0.0001f) {
            edge1_r1 = ImVec2(r1_minX, p1_r1.y);
            edge2_r1 = ImVec2(r1_maxX, p1_r1.y);
        }
        else {
            float m_r1 = (p2_r1.y - p1_r1.y) / (p2_r1.x - p1_r1.x);
            float b_r1 = p1_r1.y - m_r1 * p1_r1.x;
            
            if (abs(p2_r1.x - p1_r1.x) > abs(p2_r1.y - p1_r1.y)) {
                edge1_r1 = ImVec2(r1_minX, m_r1 * r1_minX + b_r1);
                edge2_r1 = ImVec2(r1_maxX, m_r1 * r1_maxX + b_r1);
            } else {
                edge1_r1 = ImVec2((r1_minY - b_r1) / m_r1, r1_minY);
                edge2_r1 = ImVec2((r1_maxY - b_r1) / m_r1, r1_maxY);
            }
        }
        
        drawList->AddLine(edge1_r1, edge2_r1, lineColor, 1.0f);

        // draw labels
        ImGui::SetCursorScreenPos(ImVec2((edge1_r1.x + edge2_r1.x) / 2 - 15, (edge1_r1.y + edge2_r1.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c1", line.name[0]);
        ImGui::SetCursorScreenPos(ImVec2((edge1_r2.x + edge2_r2.x) / 2 + 15, (edge1_r2.y + edge2_r2.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c2", line.name[0]);
    }
    
    // TODO: fix planes, not really working as suposed to
    // Planes can only be drawn on x+ y+ plane
    // TODO: extend the planes to the edges of the viewport like the lines
    for (const auto& plane : m_sceneData.planes) {
        const auto& p1 = m_sceneData.points[plane.point1index];
        const auto& p2 = m_sceneData.points[plane.point2index];
        const auto& p3 = m_sceneData.points[plane.point3index];

        // Calculate plane equation: Ax + By + Cz + D = 0
        glm::vec3 v1(p2.coords[0] - p1.coords[0], p2.coords[1] - p1.coords[1], p2.coords[2] - p1.coords[2]);
        glm::vec3 v2(p3.coords[0] - p1.coords[0], p3.coords[1] - p1.coords[1], p3.coords[2] - p1.coords[2]);
        glm::vec3 normal = glm::cross(v1, v2);
        
        float A = normal.x;
        float B = normal.y;
        float C = normal.z;
        float D = -(A * p1.coords[0] + B * p1.coords[1] + C * p1.coords[2]);
        
        // First vertical point (set x = 0)
        float z1_vert = (-D) / C;
        glm::vec3 vert_point1(0.0f, 0.0f, z1_vert / 3);
        
        // Second vertical point (set z = 0)
        float x1_vert = (-D) / A;
        glm::vec3 vert_point2(x1_vert / 2, 0.0f, 0.0f);

        // First horizontal point (set x = 0)
        float y1_horiz = (-D) / B;
        glm::vec3 horiz_point1(0.0f, -y1_horiz / 3, 0.0f);
        
        // Second horizontal point (set y = 0)
        float x2_horiz = (-D) / A;
        glm::vec3 horiz_point2(x2_horiz / 2, 0.0f, 0.0f);

        ImVec2 p1_horiz(cursorPos.x + viewportSize.x / 2 + horiz_point1.x * 10,
                        (cursorPos.y + viewportSize.y / 2) - horiz_point1.y * 10);
        
        ImVec2 p2_horiz(cursorPos.x + viewportSize.x / 2 + horiz_point2.x * 10,
                        (cursorPos.y + viewportSize.y / 2) + horiz_point2.y * 10);
        
        // r2 line (vertical plane)
        ImVec2 p1_vert(cursorPos.x + viewportSize.x / 2 + vert_point1.x * 10,
                        (cursorPos.y + viewportSize.y / 2) - vert_point1.z * 10);
        
        ImVec2 p2_vert(cursorPos.x + viewportSize.x / 2 + vert_point2.x * 10,
                        (cursorPos.y + viewportSize.y / 2) - vert_point2.z * 10);

        // Draw the plane lines
        drawList->AddLine(p1_horiz, p2_horiz, lineColor, 2.5f);
        drawList->AddLine(p1_vert, p2_vert, lineColor, 2.5f);
        
        // add labels
        ImGui::SetCursorScreenPos(ImVec2((p1_horiz.x + p2_horiz.x) / 2 - 15, (p1_horiz.y + p2_horiz.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c1", plane.name[0]);
        ImGui::SetCursorScreenPos(ImVec2((p1_vert.x + p2_vert.x) / 2 + 15, (p1_vert.y + p2_vert.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c2", plane.name[0]);
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void App::Run() {
    while (!glfwWindowShouldClose(m_window)) {
        Frame();
    }
}

void App::Frame(){
    glfwPollEvents();
        
    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
        
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
        
    HandleInput();

    glClearColor(
        m_sceneData.settings.backgroundColor[0], 
        m_sceneData.settings.backgroundColor[1], 
        m_sceneData.settings.backgroundColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
    int viewportWidth = m_windowWidth;
    int viewportHeight = m_windowHeight;
    int viewportX = (m_windowWidth - viewportWidth) / 2;
    int viewportY = (m_windowHeight - viewportHeight) / 2;
        
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

    DrawUI();

    m_renderer.Render();
    PrepareRenderData();
    m_renderer.UpdateCamera(m_camera, viewportWidth, viewportHeight);
        
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

void App::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}