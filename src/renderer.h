#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <vector>
#include <glm/glm.hpp>
#include "camera.h"

class Renderer {
public:
    bool Initialize();
    void Render();
    void UpdateCamera(const Camera& camera, int width, int height);

    void DrawPoints(const std::vector<glm::vec3>& points, 
                   const std::vector<glm::vec3>& colors, 
                   float size);
    void DrawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines,
                   const std::vector<glm::vec3>& colors, 
                   float thickness, const Camera& camera);
    void DrawPlanes(const std::vector<std::vector<glm::vec3>>& planes, 
                   const std::vector<glm::vec3>& colors,
                   std::vector<bool>& expand,
                   float opacity);

    void SetAxesType(int type) { m_axesType = type; }
    void SetDihedralsVisible(bool visible) { m_showDihedral = visible; }
    void SetCutPointVisible(bool visible) { m_showCutPoints = visible; }
    void SetCutLineVisible(bool visible) { m_showCutLines = visible; }

private:
    void DrawAxes();
    void SetupShaderProgram(GLuint& program, const char* vertexSrc, const char* fragmentSrc);
    void SetupBuffer(GLuint& vao, GLuint& vbo, const void* data, size_t size);

    int m_axesType = 0;
    bool m_showDihedral = false;
    bool m_showCutPoints = true;
    bool m_showCutLines = true;

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;

    GLuint m_mainShader = 0;
    GLuint m_planeShader = 0;

    GLuint m_axesVAO = 0, m_axesVBO = 0;
    GLuint m_dihedralVAO = 0, m_dihedralVBO = 0;
    GLuint m_pointVAO = 0, m_pointVBO = 0;
};