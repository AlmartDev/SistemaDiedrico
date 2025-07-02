#include "app.h"
#include "ui.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

#include "scene.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

JsonHandler* JsonHandlerInstance() {
    static JsonHandler instance;
    return &instance;
}

void handleFileLoad(const std::string& content) {
    std::cout << "File loaded with content length: " << content.length() << std::endl;
    JsonHandler* handler = JsonHandlerInstance();
    handler->loadedContent = content;
    handler->fileLoaded = true;
    
    // Write to virtual filesystem
    FILE* file = fopen(handler->loadedPath.c_str(), "w");
    if (file) {
        fwrite(content.c_str(), 1, content.length(), file);
        fclose(file);
    }
    
    // Load data through App instance
    if (App::s_instance) {
        std::vector<nlohmann::json> result = handler->Load(handler->loadedPath);
        if (!result.empty()) {
            App::s_instance->LoadProject(result);
        }
    }
}

EM_JS(void, setInitialLanguageFromURL, (), {
    const urlParams = new URLSearchParams(window.location.search);
    const langParam = urlParams.get('lang');
    if (langParam) {
        Module.setLanguage(langParam);
    }
});

void setLanguage(const std::string& lang) {
    if (App::s_instance) {
        App::s_instance->GetUI().SetLanguage(lang);
    }
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("handleFileLoad", &handleFileLoad);
    emscripten::function("setLanguage", &setLanguage);
}
#endif

double App::m_scrollY = 0.0;

App* App::s_instance = nullptr;

App::App() : m_window(nullptr) {
    m_ui = new UI();
    s_instance = this;
}

bool App::Initialize(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // Add these hints specifically for Emscripten
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Sistema Diedrico @almartdev", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    UpdateWindowTitle();

    glfwMaximizeWindow(m_window); // start maximized on native platforms

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
        app->m_windowWidth = width;
        app->m_windowHeight = height;
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->m_scrollY += yoffset;
        }
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

    // parse command line arguments to get language
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "--lang" && i + 1 < argc) {
            m_ui->SetLanguage(argv[i + 1]);
            break;
        }
    }

    #ifdef __EMSCRIPTEN__
    setInitialLanguageFromURL();
    #endif

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
        
        if (m_sceneData.settings.invertMouse[0])
            deltaX = -deltaX; 
        if (m_sceneData.settings.invertMouse[1]) 
            deltaY = -deltaY; 

        m_camera.ProcessMouseMovement(deltaX, deltaY);
        
        m_lastMouseX = static_cast<float>(mouseX);

        m_lastMouseY = static_cast<float>(mouseY);
    } else {
        m_isMousePressed = false;
    }

    if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
        glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {

        #ifdef _WIN32 // this soultion returned "illegal hardware instruction" on linux :(
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
            std::string path = m_jsonHandler.SaveFileDialog();
            m_jsonHandler.Save(path, m_sceneData);
        }
        if (glfwGetKey(m_window, GLFW_KEY_O) == GLFW_PRESS) {
            std::string path = m_jsonHandler.OpenFileDialog();
            std::vector<nlohmann::json> data = m_jsonHandler.Load(path);
            if (!data.empty()) {
                LoadProject(data);
            }
        }
        #endif
    }

    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        m_renderer.SetQuadrantLabelsVisible(!m_sceneData.settings.showQuadrantLabels);

    if (m_scrollY != 0) {
        m_camera.SetDistance(m_camera.GetDistance() - static_cast<float>(m_scrollY) * .3f);
        m_scrollY = 0.0;
    }

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
}

void App::LoadProject(std::vector<nlohmann::json> data) {
    if (data.empty()) return;

    m_sceneData.points.clear();
    m_sceneData.lines.clear();
    m_sceneData.planes.clear();

    auto getFloat = [](const nlohmann::json& j, const std::string& key, float def = 0.0f) {
        return j.contains(key) ? j[key].get<float>() : def;
    };

    GetCamera().ResetPosition();

    if (!data[0].empty()) {
        m_sceneData.settings.loadedFileName = data[0][0].value("name", "untitled");
        m_sceneData.settings.worldScale = getFloat(data[0][0], "worldScale", 50.0f);
    }

    // Load points
    if (!data[1].empty()) {
        for (const auto& point : data[1]) {
            try {
                SceneData::Point p;
                p.name = point["name"].get<std::string>();
                p.coords[0] = point["coords"]["d"].get<float>();
                p.coords[1] = point["coords"]["a"].get<float>();
                p.coords[2] = point["coords"]["c"].get<float>();
                p.color[0] = getFloat(point["color"], "0", 1.0f);
                p.color[1] = getFloat(point["color"], "1", 0.5f);
                p.color[2] = getFloat(point["color"], "2", 0.0f);
                p.hidden = point["hidden"].get<bool>();
                p.userCreated = point["userCreated"].get<bool>();
                m_sceneData.points.push_back(p);
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "Error loading point: " << e.what() << std::endl;
            }
        }
    }

    // Load lines
    if (data.size() > 1 && !data[2].empty()) {
        for (const auto& line : data[2]) {
            try {
                SceneData::Line l;
                l.name = line["name"].get<std::string>();

                // Find or create points
                auto findOrCreatePoint = [&](const nlohmann::json& pointJson, const std::string& suffix) {
                    SceneData::Point p;
                    p.name = line["name"].get<std::string>() + suffix;
                    if (pointJson.is_object()) {
                        p.coords[0] = pointJson.contains("d") ? pointJson["d"].get<float>() : 0.0f;
                        p.coords[1] = pointJson.contains("a") ? pointJson["a"].get<float>() : 0.0f;
                        p.coords[2] = pointJson.contains("c") ? pointJson["c"].get<float>() : 0.0f;
                    } else if (pointJson.is_array() && pointJson.size() >= 3) {
                        p.coords[0] = pointJson[0].get<float>();
                        p.coords[1] = pointJson[1].get<float>();
                        p.coords[2] = pointJson[2].get<float>();
                    } else {
                        p.coords[0] = p.coords[1] = p.coords[2] = 0.0f;
                    }
                    
                    // Check if point already exists
                    auto it = std::find_if(m_sceneData.points.begin(), m_sceneData.points.end(),
                        [&p](const SceneData::Point& existing) { return existing.name == p.name; });
                    
                    if (it == m_sceneData.points.end()) {
                        m_sceneData.points.push_back(p);
                        return static_cast<int>(m_sceneData.points.size() - 1);
                    }
                    return static_cast<int>(std::distance(m_sceneData.points.begin(), it));
                };

                l.point1index = findOrCreatePoint(line["point1"], "1");
                l.point2index = findOrCreatePoint(line["point2"], "2");

                l.color[0] = getFloat(line["color"], "0", 1.0f);
                l.color[1] = getFloat(line["color"], "1", 1.0f);
                l.color[2] = getFloat(line["color"], "2", 1.0f);
                l.showVisibility = line["showVisibility"].get<bool>();
                
                m_sceneData.lines.push_back(l);
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "Error loading line: " << e.what() << std::endl;
            }
        }
    }

    // Load planes
    if (data.size() > 2 && !data[3].empty()) {
        for (const auto& plane : data[3]) {
            try {
                SceneData::Plane p;
                p.name = plane["name"].get<std::string>();

                // Find or create points (similar to lines)
                auto findOrCreatePoint = [&](const nlohmann::json& pointJson, const std::string& suffix) {
                    SceneData::Point pt;
                    pt.name = plane["name"].get<std::string>() + suffix;
                    if (pointJson.is_object()) {
                        pt.coords[0] = pointJson.contains("d") ? pointJson["d"].get<float>() : 0.0f;
                        pt.coords[1] = pointJson.contains("a") ? pointJson["a"].get<float>() : 0.0f;
                        pt.coords[2] = pointJson.contains("c") ? pointJson["c"].get<float>() : 0.0f;
                    } else if (pointJson.is_array() && pointJson.size() >= 3) {
                        pt.coords[0] = pointJson[0].get<float>();
                        pt.coords[1] = pointJson[1].get<float>();
                        pt.coords[2] = pointJson[2].get<float>();
                    } else {
                        pt.coords[0] = pt.coords[1] = pt.coords[2] = 0.0f;
                    }
                    
                    auto it = std::find_if(m_sceneData.points.begin(), m_sceneData.points.end(),
                        [&pt](const SceneData::Point& existing) { return existing.name == pt.name; });
                    
                    if (it == m_sceneData.points.end()) {
                        m_sceneData.points.push_back(pt);
                        return static_cast<int>(m_sceneData.points.size() - 1);
                    }
                    return static_cast<int>(std::distance(m_sceneData.points.begin(), it));
                };

                p.point1index = findOrCreatePoint(plane["point1"], "1");
                p.point2index = findOrCreatePoint(plane["point2"], "2");
                p.point3index = findOrCreatePoint(plane["point3"], "3");

                p.color[0] = getFloat(plane["color"], "0", 0.5f);
                p.color[1] = getFloat(plane["color"], "1", 0.5f);
                p.color[2] = getFloat(plane["color"], "2", 0.5f);
                p.expand = plane["expand"].get<bool>();
                
                m_sceneData.planes.push_back(p);
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "Error loading plane: " << e.what() << std::endl;
            }
        }
    }
}

void App::PrepareRenderData() { // change from float[3] coords to glm::vec3
    std::vector<char*> pointNames, lineNames, planeNames;

    float worldScale = m_sceneData.settings.worldScale;

    std::vector<glm::vec3> pointPositions, pointColors;
    for (const auto& point : m_sceneData.points) {
        if (point.hidden) continue;
        pointNames.push_back(const_cast<char*>(point.name.c_str()));
        pointPositions.emplace_back(point.coords[0]/worldScale, point.coords[2]/worldScale, point.coords[1]/worldScale);
        pointColors.emplace_back(point.color[0], point.color[1], point.color[2]);
    }

    std::vector<std::pair<glm::vec3, glm::vec3>> linePositions;
    std::vector<glm::vec3> lineColors;
    for (const auto& line : m_sceneData.lines) {
        lineNames.push_back(const_cast<char*>(line.name.c_str()));

        const auto& point1 = m_sceneData.points[line.point1index];
        const auto& point2 = m_sceneData.points[line.point2index];

        linePositions.emplace_back(
            glm::vec3(point1.coords[0]/worldScale, point1.coords[2]/worldScale, point1.coords[1]/worldScale),
            glm::vec3(point2.coords[0]/worldScale, point2.coords[2]/worldScale, point2.coords[1]/worldScale)
        );
        lineColors.emplace_back(line.color[0], line.color[1], line.color[2]);
    }

    std::vector<std::vector<glm::vec3>> planePositions;
    std::vector<glm::vec3> planeColors;
    std::vector<bool> planeExpand;

    for (const auto& plane : m_sceneData.planes) {
        planeNames.push_back(const_cast<char*>(plane.name.c_str()));

        const auto& point1 = m_sceneData.points[plane.point1index];
        const auto& point2 = m_sceneData.points[plane.point2index];
        const auto& point3 = m_sceneData.points[plane.point3index];

        planePositions.push_back({
            glm::vec3(point1.coords[0]/worldScale, point1.coords[2]/worldScale, point1.coords[1]/worldScale),
            glm::vec3(point2.coords[0]/worldScale, point2.coords[2]/worldScale, point2.coords[1]/worldScale),
            glm::vec3(point3.coords[0]/worldScale, point3.coords[2]/worldScale, point3.coords[1]/worldScale)
        });
        planeColors.emplace_back(plane.color[0], plane.color[1], plane.color[2]);

        planeExpand.push_back(plane.expand);
    }

    m_renderer.DrawPoints(pointNames, pointPositions, pointColors, m_sceneData.settings.pointSize);
    m_renderer.DrawLines(lineNames, linePositions, lineColors, m_sceneData.settings.lineThickness, m_camera);
    m_renderer.DrawPlanes(planeNames, planePositions, planeColors, planeExpand, m_sceneData.settings.planeOpacity);
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

    if (m_sceneData.settings.VSync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }

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

    glViewport(m_sceneData.settings.offset[0], m_sceneData.settings.offset[1], width, height);

    m_ui->DrawUI(*this);

    m_renderer.Render();
    PrepareRenderData();

    //labels
    m_renderer.SetQuadrantLabelsVisible(m_sceneData.settings.showQuadrantLabels);
    m_renderer.SetLabelsVisible(m_sceneData.settings.showLabels);

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