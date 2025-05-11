#include "renderer.h"
#include <iostream>

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    GLuint s_VAO, s_VBO, s_shaderProgram;
    GLuint s_dihedralVAO, s_dihedralVBO;
    GLuint s_pointVAO, s_pointVBO;  // For point rendering

    const char* s_vertexShaderSource = R"(
        #version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        uniform mat4 view;
        uniform mat4 projection;
        uniform float pointSize;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = pointSize;
        }
    )";

    const char* s_fragmentShaderSource = R"(
        #version 300 es
        precision highp float;
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 0.6); 
        }
    )";

    const GLfloat s_axesVertices[] = {
        // 3D Axes (type 0)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // X axis (red)
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // Y axis (green)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Z axis (blue)

        // Cartesian Axes (type 1)
        -10000.0f, 0.0f, 0.0f, 10000.0f, 0.0f, 0.0f, // X axis
        0.0f, -10000.0f, 0.0f, 0.0f, 10000.0f, 0.0f, // Y axis
        0.0f, 0.0f, -10000.0f, 0.0f, 0.0f, 10000.0f  // Z axis
    };

    const GLfloat s_planeVertices[] = {
        // Horizontal plane (XZ)
        -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f,
        // Vertical plane (XY)
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
    };
}

bool Renderer::Initialize() {
    #ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    #endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &s_vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &s_fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    s_shaderProgram = glCreateProgram();
    glAttachShader(s_shaderProgram, vertexShader);
    glAttachShader(s_shaderProgram, fragmentShader);
    glLinkProgram(s_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Setup axes VAO/VBO
    glGenVertexArrays(1, &s_VAO);
    glGenBuffers(1, &s_VBO);
    glBindVertexArray(s_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_axesVertices), s_axesVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup dihedral system VAO/VBO
    glGenVertexArrays(1, &s_dihedralVAO);
    glGenBuffers(1, &s_dihedralVBO);
    glBindVertexArray(s_dihedralVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_dihedralVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_planeVertices), s_planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup point rendering VAO/VBO
    glGenVertexArrays(1, &s_pointVAO);
    glGenBuffers(1, &s_pointVBO);
    glBindVertexArray(s_pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_pointVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Renderer::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawAxes();

    if (m_showDihedral) {
        glUseProgram(s_shaderProgram);
        glBindVertexArray(s_dihedralVAO);
        
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 0.2f, 0.2f, 0.8f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 0.8f, 0.2f, 0.2f);
        glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
        
        glBindVertexArray(0);
    }

    glFlush();
}


void Renderer::DrawPoints(const std::vector<glm::vec3>& points, 
                         const std::vector<glm::vec3>& colors, 
                         float size) {
    if (points.empty() || points.size() != colors.size()) return;

    glUseProgram(s_shaderProgram);
    glUniform1f(glGetUniformLocation(s_shaderProgram, "pointSize"), size);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Draw main points
    glBindVertexArray(s_pointVAO);
    for (size_t i = 0; i < points.size(); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, s_pointVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &points[i], GL_STATIC_DRAW);
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), colors[i].r, colors[i].g, colors[i].b);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    // Draw cut points if enabled
    if (m_showCutPoints) {
        for (size_t i = 0; i < points.size(); i++) {
            std::vector<glm::vec3> cutPoints = {
                glm::vec3(points[i].x, 0.0f, points[i].z),
                glm::vec3(points[i].x, points[i].y, 0.0f)
            };

            glBindBuffer(GL_ARRAY_BUFFER, s_pointVBO);
            glBufferData(GL_ARRAY_BUFFER, cutPoints.size() * sizeof(glm::vec3), cutPoints.data(), GL_STATIC_DRAW);
            glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(cutPoints.size()));
        }
    }

    glBindVertexArray(0);
}

void Renderer::DrawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines, 
                         const std::vector<glm::vec3>& colors, 
                         float thickness) {
    
    if (lines.empty() || lines.size() != colors.size()) return;

    GLuint linesVAO, linesVBO;
    glGenVertexArrays(1, &linesVAO);
    glGenBuffers(1, &linesVBO);
    
    glUseProgram(s_shaderProgram);
    glLineWidth(thickness);

    for (size_t i = 0; i < lines.size(); i++) { // TODO: Thickness is only working for the cut lines, not the main line
        // Prepare line vertices (start and end points)
        const auto& line = lines[i];
        std::vector<glm::vec3> lineVertices = {line.first, line.second};

        // Create cut lines (projections to y=0 and z=0 planes)
        std::vector<glm::vec3> cutLines = {
            glm::vec3(line.first.x, 0.0f, line.first.z),  // y=0 start
            glm::vec3(line.second.x, 0.0f, line.second.z), // y=0 end
            glm::vec3(line.first.x, line.first.y, 0.0f),  // z=0 start
            glm::vec3(line.second.x, line.second.y, 0.0f)  // z=0 end
        };

        if (m_showCutLines) {
            // Draw cut lines (projections)
            glBindVertexArray(linesVAO);
            glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
            glBufferData(GL_ARRAY_BUFFER, cutLines.size() * sizeof(glm::vec3), cutLines.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
            glEnableVertexAttribArray(0);

            glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
            glDrawArrays(GL_LINES, 0, 2);  // y=0 projection
            glDrawArrays(GL_LINES, 2, 2);  // z=0 projection
        }

        // Draw main line
        glBindVertexArray(linesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), colors[i].r, colors[i].g, colors[i].b);
        glLineWidth(1.0f); // original line width (so it doesnt mess up with the cartesian lines) TODO: change this with a parameter
        glDrawArrays(GL_LINES, 0, 2);
    }
    
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &linesVAO);
    glDeleteBuffers(1, &linesVBO);
}

void Renderer::DrawAxes() {
    glUseProgram(s_shaderProgram);
    glBindVertexArray(s_VAO);

    if (m_axesType == 0) {
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 1.0f, 0.0f, 0.0f);
        glDrawArrays(GL_LINES, 0, 2);
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
        glDrawArrays(GL_LINES, 2, 2);
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, 4, 2);
    } 
    else if (m_axesType == 1) {
        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), 1.0f, 1.0f, 1.0f);
        glDrawArrays(GL_LINES, 6, 2);
        glDrawArrays(GL_LINES, 8, 2);
        glDrawArrays(GL_LINES, 10, 2);
    }
    else if (m_axesType == 2) {
        m_showDihedral = true;
    }

    glBindVertexArray(0);
}

void Renderer::UpdateCamera(const Camera& camera, int width, int height) {
    m_viewMatrix = camera.GetViewMatrix();
    if (height == 0) height = 1; // Prevent division by zero
    
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_projectionMatrix = glm::perspective(glm::radians(55.0f), aspectRatio, 0.1f, 100.0f);
    
    glUseProgram(s_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(s_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(s_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
}