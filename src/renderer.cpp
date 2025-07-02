#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <numeric>

#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

namespace {
    // Constants
    constexpr float DEFAULT_PLANE_SIZE = 1.1f;
    constexpr float CLAMP_VALUE = 50.0f;
    constexpr float POINT_SIZE_SCALE = 100.0f;
    constexpr float DEFAULT_OPACITY = 0.6f;
    
    // Shader sources
    const char* VERTEX_SHADER_SRC = 
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

    const char* FRAGMENT_SHADER_SRC = 
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 color;\n"
        "void main() {\n"
        "    FragColor = vec4(color, 0.6);\n"
        "}\n";

    const char* PLANE_FRAGMENT_SHADER_SRC = 
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 color;\n"
        "uniform float opacity;\n"
        "void main() {\n"
        "    FragColor = vec4(color, opacity);\n"
        "}\n";

    const GLfloat AXES_VERTICES[] = {
        // 3D Axes (type 0)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

        // Cartesian Axes (type 1)
        -10000.0f, 0.0f, 0.0f, 10000.0f, 0.0f, 0.0f,
        0.0f, -10000.0f, 0.0f, 0.0f, 10000.0f, 0.0f,
        0.0f, 0.0f, -10000.0f, 0.0f, 0.0f, 10000.0f
    };

    const GLfloat PLANE_VERTICES[] = {
        // Horizontal plane (XZ)
        -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f,
        // Vertical plane (XY)
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
    };

    void CreateThickLineGeometry(std::vector<glm::vec3>& vertices, 
                                const glm::vec3& start, 
                                const glm::vec3& end, 
                                const glm::vec3& cameraPos,
                                float thickness) {
        glm::vec3 lineDir = glm::normalize(end - start);
        glm::vec3 cameraToLine = glm::normalize((start + end) * 0.5f - cameraPos);
        glm::vec3 right = glm::normalize(glm::cross(cameraToLine, lineDir)) * thickness * 0.5f;
        glm::vec3 up = glm::normalize(glm::cross(lineDir, right)) * thickness * 0.5f;

        vertices = {
            start - right - up, start + right - up, end - right + up,
            start + right - up, end + right + up, end - right + up
        };
    }
}

bool Renderer::Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup shaders
    SetupShaderProgram(m_mainShader, VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);
    SetupShaderProgram(m_planeShader, VERTEX_SHADER_SRC, PLANE_FRAGMENT_SHADER_SRC);

    // Setup buffers
    SetupBuffer(m_axesVAO, m_axesVBO, AXES_VERTICES, sizeof(AXES_VERTICES));
    SetupBuffer(m_dihedralVAO, m_dihedralVBO, PLANE_VERTICES, sizeof(PLANE_VERTICES));
    
    // Setup point buffer
    glGenVertexArrays(1, &m_pointVAO);
    glGenBuffers(1, &m_pointVBO);
    glBindVertexArray(m_pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Renderer::SetupShaderProgram(GLuint& program, const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShader);

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Renderer::SetupBuffer(GLuint& vao, GLuint& vbo, const void* data, size_t size) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawAxes();

    if (m_showDihedral) {
        glUseProgram(m_mainShader);
        glBindVertexArray(m_dihedralVAO);
        
        glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.2f, 0.2f, 0.8f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.8f, 0.2f, 0.2f);
        glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

        // show quadrant labels
        // Coords are in dihedral space, so no x,y,z, but d,a,c (which would be like x,z,y)
        if (m_showQuadrantLabels) {
            DrawLabel("I", glm::vec3(0.0f, 0.7f, 0.7f), glm::vec3(1.0f, 1.0f, 1.0f), true);
            DrawLabel("II", glm::vec3(0.0f, 0.7f, -0.7f), glm::vec3(1.0f, 1.0f, 1.0f), true);
            DrawLabel("III", glm::vec3(0.0f, -0.7f, -0.7f), glm::vec3(1.0f, 1.0f, 1.0f), true);
            DrawLabel("IV", glm::vec3(0.0f, -0.7f, 0.7f), glm::vec3(1.0f, 1.0f, 1.0f), true);
        }
        
        if (m_showScale) {
            float scale = m_scale / 50.0f;
            DrawLabel("0", glm::vec3(-1.05f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), true);
            for (int i = -5; i <= 5; ++i) {
                if (i == 0) continue; // Skip zero
                glm::vec3 pos(-1.05f, i * 0.2f, 0.0f);
                DrawLabel(std::to_string(static_cast<int>(i * scale * 10)).c_str(), pos, glm::vec3(1.0f, 1.0f, 1.0f), false);
            }
            for (int i = -5; i <= 5; ++i) {
                if (i == 0) continue; // Skip zero
                glm::vec3 pos(-1.05f, 0.0f, i * 0.2f);
                DrawLabel(std::to_string(static_cast<int>(i * scale * 10)).c_str(), pos, glm::vec3(1.0f, 1.0f, 1.0f), false);
            }
        }

        glBindVertexArray(0);
    }

    // Draw all accumulated labels
    if (!m_labels.empty()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        
        ImGui::Begin("Labels", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoInputs | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoSavedSettings | 
            ImGuiWindowFlags_NoFocusOnAppearing | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        // Get the current window size to set as the overlay size
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        ImGui::SetWindowSize(ImVec2((float)viewport[2], (float)viewport[3]));
        ImGui::SetWindowPos(ImVec2(0, 0));
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        for (const auto& label : m_labels) {
            ImVec2 pos(std::get<1>(label).x, std::get<1>(label).y);
            const char* text = std::get<0>(label).c_str();
            ImVec2 text_size = ImGui::CalcTextSize(text);
            if (std::get<3>(label)) {
                ImVec2 padding(4.0f, 2.0f);
                ImVec2 rect_min = ImVec2(pos.x - padding.x, pos.y - padding.y);
                ImVec2 rect_max = ImVec2(pos.x + text_size.x + padding.x, pos.y + text_size.y + padding.y);
                drawList->AddRectFilled(rect_min, rect_max, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.4f)), 4.0f);
            }
            const glm::vec3& col = std::get<2>(label);
            drawList->AddText(pos, ImGui::GetColorU32(ImVec4(col.r, col.g, col.b, 1.0f)), text);
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
        m_labels.clear();
    }

    glFlush();
}

void Renderer::DrawLabel(const char* text, const glm::vec3& position, const glm::vec3& color, bool showBackground = false) {
    glm::vec2 screenPos = WorldToScreen(position);
    m_labels.emplace_back(text, screenPos, color, showBackground);
}

glm::vec2 Renderer::WorldToScreen(const glm::vec3& worldPos) {
    glm::vec4 clipSpacePos = m_projectionMatrix * m_viewMatrix * glm::vec4(worldPos, 1.0f);

    if (clipSpacePos.w == 0.0f)
        return glm::vec2(0.0f);

    glm::vec3 ndc = glm::vec3(clipSpacePos) / clipSpacePos.w;

    // Get viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    float x = viewport[0] + (ndc.x + 1.0f) * 0.5f * viewport[2];
    float y = viewport[1] + (1.0f - (ndc.y + 1.0f) * 0.5f) * viewport[3];

    return glm::vec2(x, y);
}

void Renderer::DrawPoints(const std::vector<char*>& names,
                         const std::vector<glm::vec3>& points, 
                         const std::vector<glm::vec3>& colors, 
                         float size) {
    if (points.empty() || points.size() != colors.size()) return;

    glUseProgram(m_mainShader);
    glUniform1f(glGetUniformLocation(m_mainShader, "pointSize"), size);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(m_pointVAO);
    for (size_t i = 0; i < points.size(); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &points[i], GL_STATIC_DRAW);
        glUniform3f(glGetUniformLocation(m_mainShader, "color"), colors[i].r, colors[i].g, colors[i].b);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    if (m_showPointLabels) {
        for (size_t i = 0; i < points.size(); i++) {
            DrawLabel(names[i], points[i], colors[i]);
        }
    }

    if (m_showCutPoints) {
        std::vector<glm::vec3> cutPoints;
        cutPoints.reserve(points.size() * 2);
        
        for (const auto& point : points) {
            cutPoints.emplace_back(point.x, 0.0f, point.z);
            cutPoints.emplace_back(point.x, point.y, 0.0f);
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
        glBufferData(GL_ARRAY_BUFFER, cutPoints.size() * sizeof(glm::vec3), cutPoints.data(), GL_STATIC_DRAW);
        glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.0f, 1.0f, 0.0f);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(cutPoints.size()));
    }

    glBindVertexArray(0);
}

void Renderer::DrawLines(const std::vector<char*>& names,
                         const std::vector<std::pair<glm::vec3, glm::vec3>>& lines, 
                         const std::vector<glm::vec3>& colors, 
                         float thickness, const Camera& camera) {
    if (lines.empty() || lines.size() != colors.size()) return;

    glUseProgram(m_mainShader);
    glm::vec3 cameraPos = camera.GetPosition();
    thickness /= POINT_SIZE_SCALE;

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    for (size_t i = 0; i < lines.size(); i++) {
        const auto& line = lines[i];
        std::vector<glm::vec3> vertices;

        CreateThickLineGeometry(vertices, line.first, line.second, cameraPos, thickness);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glUniform3f(glGetUniformLocation(m_mainShader, "color"), colors[i].r, colors[i].g, colors[i].b);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

        if (m_showLineLabels) {
            glm::vec3 midPoint = (line.first + line.second) * 0.5f;
            DrawLabel(names[i], midPoint, colors[i], true);
        }

        if (m_showCutLines) {
            // Disable depth test to draw cut lines over dihedrals
            glDisable(GL_DEPTH_TEST);

            CreateThickLineGeometry(vertices, 
                glm::vec3(line.first.x, line.first.y, 0.0f), 
                glm::vec3(line.second.x, line.second.y, 0.0f), 
                cameraPos, thickness);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
            glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.0f, 1.0f, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

            CreateThickLineGeometry(vertices, 
                glm::vec3(line.first.x, 0.0f, line.first.z), 
                glm::vec3(line.second.x, 0.0f, line.second.z), 
                cameraPos, thickness);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
            glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.0f, 1.0f, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

            // Restore depth test
            glEnable(GL_DEPTH_TEST);
        }
    }

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Renderer::DrawPlanes(const std::vector<char*>& names,
                         const std::vector<std::vector<glm::vec3>>& planes, 
                         const std::vector<glm::vec3>& colors,
                         std::vector<bool>& expand,
                         float opacity) {
    if (planes.empty() || planes.size() != colors.size() || planes.size() != expand.size()) return;

    glUseProgram(m_planeShader);
    glUniformMatrix4fv(glGetUniformLocation(m_planeShader, "view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(m_planeShader, "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    for (size_t i = 0; i < planes.size(); i++) {
        const auto& plane = planes[i];
        if (plane.size() < 3) continue;

        std::vector<glm::vec3> vertices;
        
        if (expand[i]) {
            glm::vec3 normal = glm::normalize(glm::cross(plane[1] - plane[0], plane[2] - plane[0]));
            float planeConstant = -glm::dot(normal, plane[0]);
            
            glm::vec3 right, forward;
            
            if (fabs(normal.y) > 0.999f) {
                right = glm::vec3(1, 0, 0);
                forward = glm::vec3(0, 0, 1);
            } else {
                right = glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
                forward = glm::normalize(glm::cross(normal, right));
            }

            glm::vec3 planeCenter = -normal * planeConstant;
            
            vertices = {
                planeCenter + (right + forward) * DEFAULT_PLANE_SIZE,
                planeCenter + (right - forward) * DEFAULT_PLANE_SIZE,
                planeCenter + (-right - forward) * DEFAULT_PLANE_SIZE,
                planeCenter + (-right + forward) * DEFAULT_PLANE_SIZE
            };
            
            for (auto& v : vertices) {
                v.x = glm::clamp(v.x, -CLAMP_VALUE, CLAMP_VALUE);
                v.y = glm::clamp(v.y, -CLAMP_VALUE, CLAMP_VALUE);
                v.z = glm::clamp(v.z, -CLAMP_VALUE, CLAMP_VALUE);
            }
        } else {
            vertices = plane;
        }

        if (m_showPlaneLabels) {
            glm::vec3 center = std::accumulate(vertices.begin(), vertices.end(), glm::vec3(0.0f)) / static_cast<float>(vertices.size());
            DrawLabel(names[i], center, colors[i], true);
        }

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glUniform3f(glGetUniformLocation(m_planeShader, "color"), colors[i].r, colors[i].g, colors[i].b);
        glUniform1f(glGetUniformLocation(m_planeShader, "opacity"), opacity);
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size()));
    }

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
}

void Renderer::DrawAxes() {
    glUseProgram(m_mainShader);
    glBindVertexArray(m_axesVAO);

    switch (m_axesType) {
        case 0: // 3D axes
            glUniform3f(glGetUniformLocation(m_mainShader, "color"), 1.0f, 0.0f, 0.0f);
            glDrawArrays(GL_LINES, 0, 2);
            glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.0f, 1.0f, 0.0f);
            glDrawArrays(GL_LINES, 2, 2);
            glUniform3f(glGetUniformLocation(m_mainShader, "color"), 0.0f, 0.0f, 1.0f);
            glDrawArrays(GL_LINES, 4, 2);
            break;
        case 1: // Cartesian axes
            glUniform3f(glGetUniformLocation(m_mainShader, "color"), 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_LINES, 6, 2);
            glDrawArrays(GL_LINES, 8, 2);
            glDrawArrays(GL_LINES, 10, 2);
            break;
        case 2: // Dihedral system
            m_showDihedral = true;
            break;
        default:
            m_showDihedral = false;
            break;
    }

    glBindVertexArray(0);
}

void Renderer::UpdateCamera(const Camera& camera, int width, int height) {
    m_viewMatrix = camera.GetViewMatrix();
    if (height == 0) height = 1;
    
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_projectionMatrix = glm::perspective(glm::radians(55.0f), aspectRatio, 0.1f, 100.0f);
    
    glUseProgram(m_mainShader);
    glUniformMatrix4fv(glGetUniformLocation(m_mainShader, "view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(m_mainShader, "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
}

glm::vec3 Renderer::SetPositionWithGuizmo(Camera& camera) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::Begin("Guizmo Overlay", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoInputs | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoFocusOnAppearing | 
        ImGuiWindowFlags_NoBringToFrontOnFocus );

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    static glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_initialGuizmoPosition / m_scale);

    ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()),
                         glm::value_ptr(camera.GetProjectionMatrix()),
                         ImGuizmo::TRANSLATE,
                         ImGuizmo::LOCAL,
                         glm::value_ptr(transform));

    ImGuizmo::Enable(true);

    ImGui::End();
    ImGui::PopStyleColor();

    glm::vec3 newPosition(transform[3].x * m_scale, transform[3].z * m_scale, transform[3].y * m_scale);
    return newPosition;
    
    /*
    return m_initialGuizmoPosition; 
    */
}