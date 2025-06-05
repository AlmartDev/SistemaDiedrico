#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float distance) : m_distance(distance) {
    ResetPosition();
}

glm::vec3 Camera::GetPosition() const {
    return m_position;
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::ProcessMouseMovement(float deltaX, float deltaY) {
    m_yaw += deltaX * m_sensitivity;
    m_pitch -= deltaY * m_sensitivity;

    // Constrain pitch to avoid flipping
    m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
    UpdatePosition();
}

void Camera::SetSensitivity(float sensitivity) {
    m_sensitivity = sensitivity;
}

void Camera::SetDistance(float distance) {
    m_distance = distance;
    UpdatePosition();
}

float Camera::GetDistance() const {
    return m_distance;
}

void Camera::ResetPosition() {
    m_yaw = 135.0f;   // Reset yaw to initial value
    m_pitch = 25.0f;  // Reset pitch to initial value
    UpdatePosition();
}

void Camera::UpdatePosition() {
    m_position.x = m_distance * cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_position.y = m_distance * sin(glm::radians(m_pitch));
    m_position.z = m_distance * sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
}