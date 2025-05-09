#include "app.h"
#include "style.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace {
    struct Point {
        std::string name;
        float coords[3];
        float color[3] = {1.0f, 0.5f, 0.0f}; // Default orange color
    };

    struct Settings {
        float backgroundColor[3] = {0.0f, 0.0f, 0.0f};
        int axesType = 1; // Default to Cartesian axes
        bool showDihedralSystem = true;
        bool showCutPoints = true;
        float mouseSensitivity = 0.2f;
        float cameraDistance = 5.5f;
        float pointSize = 8.0f;
    };

    constexpr int DEFAULT_WIDTH = 1440;
    constexpr int DEFAULT_HEIGHT = 1080;

    std::vector<Point> s_points;
    Settings s_settings;
}

App::App() : m_window(nullptr), m_isMousePressed(false), 
             m_lastMouseX(0), m_lastMouseY(0) {}

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

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL loader (GLAD)\n";
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetCustomStyle();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    if (!m_renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }
    
    return true;
}

void DrawSettingsWindow(Renderer& renderer, Camera& camera) {
    ImGui::Begin("3D Representation Settings");

    ImGui::ColorEdit3("Background Color", s_settings.backgroundColor);

    const char* axesTypes[] = {"3D Axes", "Cartesian Axes", "Dihedral Axes"};
    ImGui::Combo("Axes Type", &s_settings.axesType, axesTypes, IM_ARRAYSIZE(axesTypes));
    renderer.SetAxesType(s_settings.axesType);

    ImGui::Checkbox("Show Dihedral System", &s_settings.showDihedralSystem);
    renderer.SetDihedralsVisible(s_settings.showDihedralSystem);

    ImGui::SliderFloat("Mouse Sensitivity", &s_settings.mouseSensitivity, 0.0f, 2.0f);
    ImGui::SliderFloat("Camera Distance", &s_settings.cameraDistance, 1.0f, 20.0f);
    
    camera.SetSensitivity(s_settings.mouseSensitivity);
    camera.SetDistance(s_settings.cameraDistance);

    // button for more settings
    if (ImGui::Button("More Settings")) {
        ImGui::OpenPopup("More Settings");
    }
    if (ImGui::BeginPopupModal("More Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Not yet!");
        
        // more settings here!

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

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
            ImGui::MenuItem("Save", nullptr, false);
            ImGui::MenuItem("Load", nullptr, false);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("App")) {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("About")) {
            ImGui::Text("Version 0.2");
            ImGui::Text("Made by Alonso Martínez");
            ImGui::Text("@almartdev on GitHub");
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void DrawPointsWindow(Renderer& renderer) {
    ImGui::Begin("Point Settings");
    
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

    ImGui::End();
}

void DrawLinesWindow(Renderer& renderer) {
    ImGui::Begin("Lines Settings");

    ImGui::Text("O");

    ImGui::End();   
}

void DrawPresetWindow() {
    ImGui::Begin("Presets");
    ImGui::Text("Select a preset to load");

    if (ImGui::Button("Points")) {
        ImGui::OpenPopup("Point Presets");
    }
    if (ImGui::BeginPopupModal("Point Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Not yet!");
        
        // more settings here!

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
        ImGui::Text("Not yet!");
        
        // more settings here!

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
        ImGui::Text("Not yet!");
        
        // more settings here!

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Text("Selected Preset: [PLACEHOLDER]");
    if (ImGui::Button("Load!")) {
        // nothing now
    }

    ImGui::End();
}

void App::DrawDihedralViewport() { // TODO: this whole function should be maybe on renderer class or even on a new class
    // Push custom style for this window only
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); // Square corners
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f)); // Dark background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Light border
        
    ImGui::SetNextWindowSize(ImVec2(500, 350));
    ImGui::Begin("Dihedral Projection", nullptr, 
                ImGuiWindowFlags_NoCollapse | 
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoDecoration |     
                ImGuiWindowFlags_NoTitleBar);
    // Get the size of the window
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    // Ground line (L.T.) //
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 p0 = ImVec2(cursorPos.x, cursorPos.y + viewportSize.y / 2);
    ImVec2 p1 = ImVec2(cursorPos.x + viewportSize.x, cursorPos.y + viewportSize.y / 2);
    drawList->AddLine(p0, p1, IM_COL32(255, 255, 255, 255), 2.0f); 

    // draw the two little lines under each side of the ground line that explain that that is the ground line
    drawList->AddLine(ImVec2(p0.x + 4, p0.y + 5), ImVec2(p0.x + 25, p0.y + 5), IM_COL32(255, 255, 255, 255), 2.0f);
    drawList->AddLine(ImVec2(p1.x - 4, p1.y + 5), ImVec2(p1.x - 25, p1.y + 5), IM_COL32(255, 255, 255, 255), 2.0f);

    // Origin point
    // commented because I dont like how it looks right now, but it works!
    /*
    ImVec2 p2 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 - 5);
    ImVec2 p3 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 + 5);
    drawList->AddLine(p2, p3, IM_COL32(255, 255, 255, 255), 2.0f); // L.T. line
    */
    
    // Draw points in 2D projection
    for (const auto& point : s_points) {

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
        ImGui::SetCursorScreenPos(ImVec2(pos2.x - 20, pos2.y - 20)); // P1
        ImGui::Text("%c1", point.name[0]);
        ImGui::SetCursorScreenPos(ImVec2(pos1.x - 20, pos1.y - 20)); // P2
        ImGui::Text("%c2", point.name[0]);

        // Draw projection lines
        drawList->AddLine(pos1, pos2, IM_COL32(255, 255, 255, 100), 1.0f);
    }
    
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void App::Run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

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
                m_lastMouseX = mouseX;
                m_lastMouseY = mouseY;
            }
            
            float deltaX = mouseX - m_lastMouseX;
            float deltaY = mouseY - m_lastMouseY;
            
            m_camera.ProcessMouseMovement(deltaX, deltaY);
            
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        } else {
            m_isMousePressed = false;
        }

        // Clear screen
        glClearColor(s_settings.backgroundColor[0], 
                    s_settings.backgroundColor[1], 
                    s_settings.backgroundColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw UI
        DrawMenuBar(m_window);
        DrawSettingsWindow(m_renderer, m_camera);

        DrawPresetWindow();

        DrawPointsWindow(m_renderer);
        DrawLinesWindow(m_renderer);
        
        DrawDihedralViewport(); // Draw the new 2D viewport

        // Prepare point data
        std::vector<glm::vec3> pointPositions, pointColors;
        for (const auto& point : s_points) {
            pointPositions.emplace_back(point.coords[0]/50, point.coords[2]/50, point.coords[1]/50);
            pointColors.emplace_back(point.color[0], point.color[1], point.color[2]);
        }

        // Render scene
        m_renderer.Render();
        m_renderer.DrawPoints(pointPositions, pointColors, s_settings.pointSize);
        m_renderer.UpdateCamera(m_camera, DEFAULT_WIDTH, DEFAULT_HEIGHT);
        
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