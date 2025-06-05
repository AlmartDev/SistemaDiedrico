#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float distance = 4.5f);
    
    glm::vec3 GetPosition() const;
    glm::mat4 GetViewMatrix() const;

    void ProcessMouseMovement(float deltaX, float deltaY);
    void SetSensitivity(float sensitivity);
    void SetDistance(float distance);
    float GetDistance() const;

    void ResetPosition();

private:
    void UpdatePosition();

    float m_sensitivity = 0.5f;
    glm::vec3 m_position;
    float m_distance;

    float initialYaw = 135.0f; // Initial yaw
    float initialPitch = 25.0f; // Initial pitch

    float m_yaw = 0.0f;   // Initial yaw
    float m_pitch = 0.0f;   // Initial pitch
};

#endif // CAMERA_H