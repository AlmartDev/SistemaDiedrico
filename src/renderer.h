#pragma once

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

    void SetAxesType(int type) { m_axesType = type; }
    void SetDihedralsVisible(bool visible) { m_showDihedral = visible; }
    void SetCutPointVisible(bool visible) { m_showCutPoints = visible; }

private:
    void DrawAxes();

    int m_axesType = 0;
    bool m_showDihedral = false;
    bool m_showCutPoints = true;

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
};