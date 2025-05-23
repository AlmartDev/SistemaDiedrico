#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float distance = 6.5f); // default distance if not specified on app.cpp
    
    glm::vec3 GetPosition() const;
    glm::mat4 GetViewMatrix() const;

    void ProcessMouseMovement(float deltaX, float deltaY);
    void SetSensitivity(float sensitivity);
    void SetDistance(float distance);
    float GetDistance() const;

private:
    void UpdatePosition();

    float m_sensitivity = 0.5f;
    glm::vec3 m_position;
    float m_distance;
    float m_yaw = 135.0f;   // Initial yaw
    float m_pitch = 25.0f;   // Initial pitch
};

#endif // CAMERA_H