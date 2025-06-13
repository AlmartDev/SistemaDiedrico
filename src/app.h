// app.h
#pragma once

#include "renderer.h"
#include "camera.h"
#include "json.h"
#include "scene.h"

// Forward declaration instead of including ui.h
class UI;

#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <GLFW/glfw3.h>
#else
    #include <glad/glad.h>
    #include <GLFW/glfw3.h>
#endif

class App {
public:
    static App* s_instance;

    App();

    bool Initialize(int argc = 0, char** argv = nullptr);
    void Run();
    void Frame();
    void Shutdown();

    GLFWwindow* GetWindow() const { return m_window; }
    SceneData& GetSceneData() { return m_sceneData; }
    Camera& GetCamera() { return m_camera; }
    Renderer& GetRenderer() { return m_renderer; }
    UI& GetUI() { return *m_ui; }

    void SetSceneData(const SceneData& sceneData) { // in case we open a new project we get rid of the old one
        m_sceneData = sceneData;
    }

    void HandleInput();
    void PrepareRenderData();

    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }

    JsonHandler& GetJsonHandler() { return m_jsonHandler; }
    
    static double m_scrollY;

    void LoadProject(std::vector<nlohmann::json> data);
    
    void DeletePoint(SceneData::Point& point);
private:
    // ui class
    UI* m_ui;

    GLFWwindow* m_window;
    Renderer m_renderer;
    Camera m_camera;
    JsonHandler m_jsonHandler;

    SceneData m_sceneData;

    const int DEFAULT_WIDTH = 1920;
    const int DEFAULT_HEIGHT = 1080;
    int m_windowWidth = DEFAULT_WIDTH;
    int m_windowHeight = DEFAULT_HEIGHT;

    bool m_isMousePressed = false;
    float m_lastMouseX = 0;
    float m_lastMouseY = 0;
    bool m_jsonLoaded = false;
};