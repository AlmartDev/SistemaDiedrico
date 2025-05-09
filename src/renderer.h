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
    void DrawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines,
                    const std::vector<glm::vec3>& colors, 
                    float thickness);

    // void DrawPlanes(); // not yet implemented

    void SetAxesType(int type) { m_axesType = type; }
    void SetDihedralsVisible(bool visible) { m_showDihedral = visible; }
    
    void SetCutPointVisible(bool visible) { m_showCutPoints = visible; }
    void SetCutLineVisible(bool visible) { m_showCutLines = visible; }

private:
    void DrawAxes();

    int m_axesType = 0;
    bool m_showDihedral = false;

    bool m_showCutPoints = true;
    bool m_showCutLines = true;

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
};