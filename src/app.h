#ifndef APP_H
#define APP_H

#include "renderer.h"
#include "camera.h"
#include "json.h"

#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <GLFW/glfw3.h>
#else
    #include <glad/glad.h>
    #include <GLFW/glfw3.h>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class App {
public:
    App();
    ~App();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    void DrawDihedralViewport();

    GLFWwindow* m_window;
    Renderer m_renderer;
    Camera m_camera;

    JsonHandler m_jsonHandler;
    bool m_jsonLoaded = false;

    ImFont* font;

    int DEFAULT_WIDTH = 1440;
    int DEFAULT_HEIGHT = 1080;

    int m_windowWidth = DEFAULT_WIDTH;
    int m_windowHeight = DEFAULT_HEIGHT;

    // Input state
    bool m_isMousePressed;
    float m_lastMouseX;
    float m_lastMouseY;
};

#endif // APP_H