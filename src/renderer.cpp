#include "Renderer.h"
#include "Camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Global OpenGL objects
GLuint VAO, VBO, shaderProgram;
GLuint dihedralVAO, dihedralVBO; // For dihedral system

bool Renderer::Init() {
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return false;
    }

    // Enable depth testing and blending
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Vertex shader source
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader source
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, .6f); 
        }
    )";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Create shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Axes vertices data
    GLfloat vertices[] = {
        // 3D Axes (type 0)
        // X axis (red)
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        // Y axis (green)
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        // Z axis (blue)
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,

        // Cartesian Axes (type 1)
        // X axis (white)
        -10000.0f, 0.0f, 0.0f,
        10000.0f, 0.0f, 0.0f,
        // Y axis (white)
        0.0f, -10000.0f, 0.0f,
        0.0f, 10000.0f, 0.0f,
        // Z axis (white)
        0.0f, 0.0f, -10000.0f,
        0.0f, 0.0f, 10000.0f
    };

    // Setup axes VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Dihedral system vertices (planes)
    GLfloat planeVertices[] = {
        // Horizontal plane (XZ plane, y=0)
        -1.0f, 0.0f, -1.0f,  // bottom-left
         1.0f, 0.0f, -1.0f,  // bottom-right
         1.0f, 0.0f,  1.0f,  // top-right
        -1.0f, 0.0f,  1.0f,  // top-left

        // Vertical plane (XY plane, z=0)
        -1.0f, -1.0f, 0.0f,  // bottom-left
         1.0f, -1.0f, 0.0f,  // bottom-right
         1.0f,  1.0f, 0.0f,  // top-right
        -1.0f,  1.0f, 0.0f   // top-left
    };

    // Setup dihedral system VAO and VBO
    glGenVertexArrays(1, &dihedralVAO);
    glGenBuffers(1, &dihedralVBO);

    glBindVertexArray(dihedralVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dihedralVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Renderer::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the axes
    drawAxes();

    // DIHEDRAL SYSTEM HERE - draw projection planes
    if (m_isDihedral) {
        glUseProgram(shaderProgram);
        glBindVertexArray(dihedralVAO);
        
        // Set color and draw horizontal plane (XZ plane, blue)
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.2f, 0.8f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        // Set color and draw vertical plane (XY plane, red)
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.8f, 0.2f, 0.2f);
        glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
        
        glBindVertexArray(0);
    }

    glFlush();
}

void Renderer::DrawPoints(const std::vector<glm::vec3>& points, const std::vector<glm::vec3>& colors) {
    if (points.empty()) return;
    if (points.size() != colors.size()) return;

    GLuint pointsVAO, pointsVBO, colorsVBO;
    glGenVertexArrays(1, &pointsVAO);
    glGenBuffers(1, &pointsVBO);
    glGenBuffers(1, &colorsVBO);
    
    // Use the shader program
    glUseProgram(shaderProgram);
    
    // Enable point sprites and set size
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(8.0f);

    for (size_t i = 0; i < points.size(); i++) { // for every point we draw the cut points and the point itself
        // Process current point
        std::vector<glm::vec3> currentPoint = {points[i]};
        std::vector<glm::vec3> currentColor = {colors[i]};
        std::vector<glm::vec3> cutPoints;

        // Create cut points for current point
        cutPoints.push_back(glm::vec3(points[i].x, 0.0f, points[i].z)); // cut with y=0
        cutPoints.push_back(glm::vec3(points[i].x, points[i].y, 0.0f));  // cut with z=0

        // First draw the cut points (always green) if enabled
        if (m_isCutPoint) {
            glBindVertexArray(pointsVAO);
            glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
            glBufferData(GL_ARRAY_BUFFER, cutPoints.size() * sizeof(glm::vec3), cutPoints.data(), GL_STATIC_DRAW);
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // For cut points, use uniform color (green)
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(cutPoints.size()));
        }    
        
        // Now draw the original point with its individual color
        glBindVertexArray(pointsVAO);
        
        // Bind and upload point position
        glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
        glBufferData(GL_ARRAY_BUFFER, currentPoint.size() * sizeof(glm::vec3), currentPoint.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Bind and upload color
        glBindBuffer(GL_ARRAY_BUFFER, colorsVBO);
        glBufferData(GL_ARRAY_BUFFER, currentColor.size() * sizeof(glm::vec3), currentColor.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        // Use individual color for the point
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), colors[i].r, colors[i].g, colors[i].b);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(currentPoint.size()));
    }
    
    // Cleanup
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &pointsVAO);
    glDeleteBuffers(1, &pointsVBO);
    glDeleteBuffers(1, &colorsVBO);
}

void Renderer::drawAxes() {
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    if (m_axesType == 0) {
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.0f, 0.0f); // red
        glBindVertexArray(VAO);
        
        glDrawArrays(GL_LINES, 0, 2); // Draw X axis
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 0.0f); // green
        glDrawArrays(GL_LINES, 2, 2); // Draw Y axis
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 0.0f, 1.0f); // blue
        glDrawArrays(GL_LINES, 4, 2); // Draw Z axis
    } else if (m_axesType == 1) {
        // Draw Cartesian axes
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f); // White
        glDrawArrays(GL_LINES, 6, 2); // X
        glDrawArrays(GL_LINES, 8, 2); // Y
        glDrawArrays(GL_LINES, 10, 2); // Z
    } else if (m_axesType == 2) {
        m_isDihedral = true; // Dihedral axes
    }

    glBindVertexArray(0);
}

void Renderer::UpdateCamera(const Camera& camera, int width, int height) {
    viewMatrix = camera.GetViewMatrix();
    
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        aspectRatio,
        0.1f,
        100.0f
    );

    // Update shader uniforms
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"), 
        1, GL_FALSE, 
        glm::value_ptr(viewMatrix)
    );
    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"), 
        1, GL_FALSE, 
        glm::value_ptr(projectionMatrix)
    );
}