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

    constexpr int DEFAULT_WIDTH = 1600;
    constexpr int DEFAULT_HEIGHT = 1200;

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
    renderer.SetDihedralSystemVisible(s_settings.showDihedralSystem);

    ImGui::SliderFloat("Mouse Sensitivity", &s_settings.mouseSensitivity, 0.0f, 2.0f);
    ImGui::SliderFloat("Camera Distance", &s_settings.cameraDistance, 1.0f, 20.0f);
    
    camera.SetSensitivity(s_settings.mouseSensitivity);
    camera.SetDistance(s_settings.cameraDistance);

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
            ImGui::Text("Version 0.1");
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
    
    for (size_t i = 0; i < s_points.size(); ++i) {
        auto& point = s_points[i];
        ImVec4 color(point.color[0], point.color[1], point.color[2], 1.0f);

        ImGui::Text("%s", point.name.c_str());
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
        DrawPointsWindow(m_renderer);

        // Prepare point data
        std::vector<glm::vec3> pointPositions, pointColors;
        for (const auto& point : s_points) {
            pointPositions.emplace_back(point.coords[0]/10, point.coords[2]/10, point.coords[1]/10);
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