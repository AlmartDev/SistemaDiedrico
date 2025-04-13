#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float distance = 3.0f) 
        : m_distance(distance), 
          m_yaw(-90.0f), 
          m_pitch(0.0f) {
        UpdatePosition();
    }

    glm::vec3 GetPosition() const { return m_position; }
    glm::mat4 GetViewMatrix() const { 
        return glm::lookAt(m_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
    }

    void ProcessMouseMovement(float deltaX, float deltaY);

    void SetSensitivity(float newSensitivity) { m_sensitivity = newSensitivity; }
    void SetDistance(float newDistance) { 
        m_distance = newDistance; 
        UpdatePosition(); 
    }

    void GetDistance(float &distance) const { distance = m_distance; }

private:
    void UpdatePosition();

    float m_sensitivity = 0.5f;

    glm::vec3 m_position;
    float m_distance;  // Distance from origin
    float m_yaw;       // Horizontal rotation (left/right)
    float m_pitch;     // Vertical rotation (up/down)
};

#endif