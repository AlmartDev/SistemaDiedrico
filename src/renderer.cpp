#include "renderer.h"
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

namespace {
    GLuint s_VAO, s_VBO, s_shaderProgram;
    GLuint s_dihedralVAO, s_dihedralVBO;
    GLuint s_pointVAO, s_pointVBO;  // For point rendering
    GLuint s_planeVAO, s_planeVBO;  // For plane rendering

    const char* s_vertexShaderSource = 
        "#version 300 es\n"
        "precision highp float;\n"
        "layout(location = 0) in vec3 aPos;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform float pointSize;\n"
        "void main() {\n"
        "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
        "    gl_PointSize = pointSize;\n"
        "}\n";

    const char* s_fragmentShaderSource = 
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 color;\n"
        "void main() {\n"
        "    FragColor = vec4(color, 0.6);\n"
        "}\n";

    const char* s_planeFragmentShaderSource = 
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 color;\n"
        "uniform float opacity;\n"
        "void main() {\n"
        "    FragColor = vec4(color, opacity);\n"
        "}\n";

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

    GLuint planeFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(planeFragmentShader, 1, &s_planeFragmentShaderSource, nullptr);
    glCompileShader(planeFragmentShader);

    // Create main shader program
    s_shaderProgram = glCreateProgram();
    glAttachShader(s_shaderProgram, vertexShader);
    glAttachShader(s_shaderProgram, fragmentShader);
    glLinkProgram(s_shaderProgram);

    // Create plane shader program (same vertex shader but different fragment shader)
    GLuint planeShaderProgram = glCreateProgram();
    glAttachShader(planeShaderProgram, vertexShader);
    glAttachShader(planeShaderProgram, planeFragmentShader);
    glLinkProgram(planeShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(planeFragmentShader);

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

    // Setup plane rendering VAO/VBO
    glGenVertexArrays(1, &s_planeVAO);
    glGenBuffers(1, &s_planeVBO);
    glBindVertexArray(s_planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_planeVertices), s_planeVertices, GL_STATIC_DRAW);
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
    for (size_t i = 0; i < lines.size(); i++) {
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

        // Draw main line with thickness
        glLineWidth(thickness);
        glBindVertexArray(linesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glUniform3f(glGetUniformLocation(s_shaderProgram, "color"), colors[i].r, colors[i].g, colors[i].b);
        glDrawArrays(GL_LINES, 0, 2);

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

        // Reset line width
        glLineWidth(1.0f);
    }
    
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &linesVAO);
    glDeleteBuffers(1, &linesVBO);
}

void Renderer::DrawPlanes(const std::vector<std::vector<glm::vec3>>& planes, 
                         const std::vector<glm::vec3>& colors,
                         std::vector<bool>& expand,
                         float opacity) {
    if (planes.empty() || planes.size() != colors.size() || planes.size() != expand.size()) return;

    // Create a temporary shader program for planes
    GLuint planeShaderProgram = glCreateProgram();
    
    // Recompile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &s_vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // Recompile plane fragment shader
    GLuint planeFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(planeFragmentShader, 1, &s_planeFragmentShaderSource, nullptr);
    glCompileShader(planeFragmentShader);
    
    glAttachShader(planeShaderProgram, vertexShader);
    glAttachShader(planeShaderProgram, planeFragmentShader);
    glLinkProgram(planeShaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(planeFragmentShader);

    glUseProgram(planeShaderProgram);
    
    // Set view and projection matrices
    glUniformMatrix4fv(glGetUniformLocation(planeShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(planeShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
    
    // Enable depth testing but allow semi-transparent planes
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < planes.size(); i++) {
        const auto& plane = planes[i];
        if (plane.size() < 3) continue;

        std::vector<glm::vec3> vertices;
        
        if (expand[i]) {
            glm::vec3 normal = glm::normalize(glm::cross(plane[1] - plane[0], plane[2] - plane[0]));
            float planeConstant = -glm::dot(normal, plane[0]); // d in plane equation
            
            glm::vec3 right, forward;
            
            // Handle case where normal is nearly vertical
            if (fabs(normal.y) > 0.999f) {
                right = glm::vec3(1, 0, 0);
                forward = glm::vec3(0, 0, 1);
            } else {
                right = glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
                forward = glm::normalize(glm::cross(normal, right));
            }

            float size = 1.1f; // size of expanded plane
            
            glm::vec3 planeCenter = -normal * planeConstant; // closest point on plane to origin
            
            vertices = {
                planeCenter + (right + forward) * size,
                planeCenter + (right - forward) * size,
                planeCenter + (-right - forward) * size,
                planeCenter + (-right + forward) * size
            };
            
            for (auto& v : vertices) {
                v.x = glm::clamp(v.x, -50.0f, 50.0f);
                v.y = glm::clamp(v.y, -50.0f, 50.0f);
                v.z = glm::clamp(v.z, -50.0f, 50.0f);
            }
        } else {
            vertices = plane;
        }

        GLuint planeVAO, planeVBO;
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        
        // Set color and opacity
        glUniform3f(glGetUniformLocation(planeShaderProgram, "color"), 
                   colors[i].r, colors[i].g, colors[i].b);
        glUniform1f(glGetUniformLocation(planeShaderProgram, "opacity"), opacity);
        
        // Draw the plane
        if (expand[i]) {
            glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size()));
        } else {
            glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size()));
        }
        
        // Clean up
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &planeVAO);
        glDeleteBuffers(1, &planeVBO);
    }

    glDeleteProgram(planeShaderProgram);
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