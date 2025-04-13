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

    bool Init(); 
    void Run();  
    void Shutdown();  
private:

    GLFWwindow* window;  // GLFW window pointer
    Renderer renderer;  // Renderer object

    Camera camera;  // Camera object

    // input for camera control
    bool isMousePressed;  // Flag for mouse press state
    float lastX, lastY;
    double scrollY;
};

#endif // APP_H
