#ifndef APP_H
#define APP_H

#include "renderer.h"
#include "camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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
    GLFWwindow* m_window;
    Renderer m_renderer;
    Camera m_camera;

    // Input state
    bool m_isMousePressed;
    float m_lastMouseX;
    float m_lastMouseY;
};

#endif // APP_H