// app.cpp
#include "app.h"
#include "style.h"

#include <vector>
#include <iostream>
#include <algorithm>

#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Application state variables
namespace {
    struct Point {
        std::string name;
        float coords[3];
        float color[3] = {1.0f, 0.5f, 0.0f}; // Default orange color for points
    };

    // Global settings
    struct {
        float backgroundColor[3] = {0.0f, 0.0f, 0.0f};
        int axesType = 1; // Default axes type
        bool showDihedralSystem = true;
        bool showCutPoints = true;
        float mouseSensitivity = 0.2f;
        float cameraDistance = 5.5f;
    } settings;

    int width = 1600;  // Default width
    int height = 1200; // Default height

    std::vector<Point> points; // Collection of user-defined points
}

App::App() : window(nullptr), isMousePressed(false), lastX(0), lastY(0) {}
App::~App() = default;

bool App::Init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return false;
    }

    window = glfwCreateWindow(width, height, "Sistema Diedrico", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error creating GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL loader (GLAD)\n";
        return false;
    }

    // Configure OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    setStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Initialize renderer and camera
    if (!renderer.Init()) {
        std::cerr << "Renderer initialization failed\n";
        return false;
    }
    
    return true;
}

void DrawSettingsWindow(Renderer& renderer, Camera& camera) {
    ImGui::Begin("Ajustes representación 3D");

    ImGui::Text("Background Color");
    ImGui::ColorEdit3("Color", settings.backgroundColor);

    ImGui::Text("Axes Type");
    const char* axesTypes[] = {"3D Axes", "Cartesian Axes", "Only Dihedral Axes"};
    ImGui::Combo("Type", &settings.axesType, axesTypes, IM_ARRAYSIZE(axesTypes));
    renderer.SetAxesType(settings.axesType);

    ImGui::Text("Visivibilidad del Sistema Diedrico");
    ImGui::Checkbox("Mostrar Diedros", &settings.showDihedralSystem);
    renderer.SetDihedralSystemVisible(settings.showDihedralSystem);

    ImGui::Text("Camera Settings");
    ImGui::SliderFloat("Mouse Sensitivity", &settings.mouseSensitivity, 0.0f, 2.0f);
    ImGui::SliderFloat("Camera Distance", &settings.cameraDistance, 1.0f, 20.0f);
    
    camera.SetSensitivity(settings.mouseSensitivity);
    camera.SetDistance(settings.cameraDistance);

    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", 
                camera.GetPosition().x, 
                camera.GetPosition().y, 
                camera.GetPosition().z);

    // FPS counter
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
    ImGui::Begin("Ajustes representación puntos");
    
    static char pointName[128] = "";
    static float pointCoords[3] = {0.0f, 0.0f, 0.0f};

    ImGui::InputText("Point Name", pointName, sizeof(pointName));
    ImGui::InputFloat3("Point Coordinates", pointCoords);

    if (ImGui::Button("Add Point")) {
        std::string trimmedName = pointName;
        trimmedName.erase(trimmedName.find_last_not_of(" \t\n\r\f\v") + 1);
        trimmedName.erase(0, trimmedName.find_first_not_of(" \t\n\r\f\v"));

        if (trimmedName.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error");
        } 
        else if (std::any_of(points.begin(), points.end(), 
                            [&](const Point& p) { return p.name == trimmedName; })) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error");
        } 
        else {
            points.push_back({trimmedName, {pointCoords[0], pointCoords[1], pointCoords[2]}});
            pointName[0] = '\0';
            memset(pointCoords, 0, sizeof(pointCoords));
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show Cut Points", &settings.showCutPoints);
    renderer.SetCutPointVisible(settings.showCutPoints);

    ImGui::Separator();
    
    for (size_t i = 0; i < points.size(); ++i) {
        auto& point = points[i];
        ImVec4 pointColor = ImVec4(point.color[0], point.color[1], point.color[2], 1.0f);

        ImGui::Text("%s", point.name.c_str());
        ImGui::SameLine();

        ImGui::PushID(static_cast<int>(i));
        ImGui::DragFloat3("", point.coords, 0.1f);
        ImGui::SameLine();

        ImGui::ColorEdit4("pointColor", (float*)&pointColor, 
                         ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

        point.color[0] = pointColor.x;
        point.color[1] = pointColor.y;
        point.color[2] = pointColor.z;

        ImGui::SameLine();
        if (ImGui::Button("X")) {
            points.erase(points.begin() + i);
            --i;
        }
        ImGui::PopID();
    }

    ImGui::End();
}

void App::Run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Handle mouse input for camera control
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            if (!isMousePressed) {
                isMousePressed = true;
                lastX = mouseX;
                lastY = mouseY;
            }
            
            float deltaX = mouseX - lastX;
            float deltaY = mouseY - lastY;
            
            camera.ProcessMouseMovement(deltaX, deltaY);
            
            lastX = mouseX;
            lastY = mouseY;
        } 
        else {
            isMousePressed = false;
        }

        // Clear screen
        glClearColor(settings.backgroundColor[0], 
                    settings.backgroundColor[1], 
                    settings.backgroundColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw UI elements
        DrawMenuBar(window);
        DrawSettingsWindow(renderer, camera);
        DrawPointsWindow(renderer);

        if (settings.showDihedralSystem) { // debug!!! we should move this out of here
            
            
        }

        // Prepare point data for rendering
        std::vector<glm::vec3> glmPoints, glmColors;
        for (const auto& point : points) {
            glmPoints.emplace_back(point.coords[0]/10, point.coords[2]/10, point.coords[1]/10);
            glmColors.emplace_back(point.color[0], point.color[1], point.color[2]);
        }

        // Render 3D scene
        renderer.Render();
        renderer.DrawPoints(glmPoints, glmColors);
        renderer.UpdateCamera(camera, width, height);

        // Render ImGui and swap buffers
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void App::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}