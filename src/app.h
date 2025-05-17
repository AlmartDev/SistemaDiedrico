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

#include "scene.h"

class App {
public:
    App();
    ~App() = default;

    bool Initialize();
    void Run();
    void Shutdown();

private:
    void DrawDihedralViewport();

    void HandleInput();
    void PrepareRenderData();
    
    void SetupImGui();
    void DrawUI();

    // UI Drawing functions
    void DrawMenuBar();
    void DrawSettingsWindow();
    
    void DrawPresetWindow();
    
    void DrawTabsWindow();
    void DrawPointsTab();
    void DrawLinesTab();
    void DrawPlanesTab();

    void DeletePoint(SceneData::Point& point);
    
    GLFWwindow* m_window;
    Renderer m_renderer;
    Camera m_camera;
    JsonHandler m_jsonHandler;
    ImFont* m_font;

    SceneData m_sceneData;
    static double m_scrollY;

    const int DEFAULT_WIDTH = 1440;
    const int DEFAULT_HEIGHT = 1080;
    int m_windowWidth = DEFAULT_WIDTH;
    int m_windowHeight = DEFAULT_HEIGHT;

    bool m_isMousePressed = false;
    float m_lastMouseX = 0;
    float m_lastMouseY = 0;
    bool m_jsonLoaded = false;
};