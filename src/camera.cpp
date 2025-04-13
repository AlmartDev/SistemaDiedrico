#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

void Camera::ProcessMouseMovement(float deltaX, float deltaY) {
    m_yaw += deltaX * m_sensitivity;
    m_pitch -= deltaY * m_sensitivity;

    // Prevent camera from flipping upside down
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    UpdatePosition();
}

void Camera::UpdatePosition() {
    // Convert spherical coordinates (yaw, pitch, distance) to Cartesian (x,y,z)
    m_position.x = m_distance * cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_position.y = m_distance * sin(glm::radians(m_pitch));
    m_position.z = m_distance * sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
}