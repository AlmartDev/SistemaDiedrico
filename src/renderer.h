#pragma once

#include <vector>

#include "Camera.h"
#include <glm/glm.hpp>

class Renderer {
public:
    bool Init();
    void Render();
    void UpdateCamera(const Camera& camera, int width, int height);

    void Renderer::SetAxesType(int type) { m_axesType = type; }
    void Renderer::SetDihedralSystemVisible(bool visible) { m_isDihedral = visible; }
    void SetCutPointVisible(bool visible) { m_isCutPoint = visible; }

    void DrawPoints(const std::vector<glm::vec3>& points, const std::vector<glm::vec3>& colors);
    // void DrawLines
    // void DrawPlanes

private:
    void drawAxes();

    int m_axesType = 0;
    bool m_isDihedral = false;
    bool m_isCutPoint = true; // show cut points

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};