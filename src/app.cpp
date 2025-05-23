#include "app.h"
#include "ui.h"
#include "style.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

#include "scene.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten.h>
#endif

double App::m_scrollY = 0.0;

App::App() : m_window(nullptr), m_jsonHandler("./assets/presets.json") {
    m_ui = new UI();
}

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_ui->SetupImGui(*this);

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

void App::HandleInput() {
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);

    // feedback: PEOPLE USE LEFT CLICK TO ROTATE????? 
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

    /*if (m_scrollY != 0) {
        m_camera.SetDistance(m_camera.GetDistance() - static_cast<float>(m_scrollY) * .3f);
        m_scrollY = 0.0;
    }*/

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
}

void App::PrepareRenderData() { // change from float[3] coords to glm::vec3
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

void App::DeletePoint(SceneData::Point& point) { // this isnt good
    point.hidden = true;
    point.name = "deleted";
}

void App::Run() {
    while (!glfwWindowShouldClose(m_window)) {
        Frame();
    }
}

void App::Frame() {
    glfwPollEvents();
        
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (m_sceneData.settings.VSync) 
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);

    // Get the display size first (this is crucial for Emscripten)
#ifdef __EMSCRIPTEN__
    // Get the actual canvas size from JavaScript
    int displayWidth = EM_ASM_INT({
        return Math.floor(Module.canvas.clientWidth * (window.devicePixelRatio || 1));
    });
    int displayHeight = EM_ASM_INT({
        return Math.floor(Module.canvas.clientHeight * (window.devicePixelRatio || 1));
    });
    
    // Update GLFW's window size to match the canvas
    glfwSetWindowSize(m_window, displayWidth, displayHeight);
    
    // Update ImGui's display size
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayWidth, (float)displayHeight);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
#endif

    HandleInput();

    glClearColor(
        m_sceneData.settings.backgroundColor[0], 
        m_sceneData.settings.backgroundColor[1], 
        m_sceneData.settings.backgroundColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get the framebuffer size (should match display size)
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    m_renderer.UpdateCamera(m_camera, width, height);

    glViewport(m_sceneData.settings.offset[0], m_sceneData.settings.offset[1], 
        m_sceneData.settings.offset[0] + width, m_sceneData.settings.offset[1] + height);

    m_ui->DrawUI(*this);

    m_renderer.Render();
    PrepareRenderData();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
    glfwSwapBuffers(m_window);
}

void App::Shutdown() {
    if (m_ui) {
        m_ui->ShutdownImGui();
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();
}