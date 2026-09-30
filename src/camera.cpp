#include "camera.h"
#include <algorithm>

namespace voxel {

Camera::Camera(glm::vec3 position, float yaw, float pitch)
    : m_position(position), m_yaw(yaw), m_pitch(pitch),
      m_worldUp(glm::vec3(0.0f, 1.0f, 0.0f)) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    switch (direction) {
        case FORWARD:  m_position += m_front * velocity; break;
        case BACKWARD: m_position -= m_front * velocity; break;
        case LEFT:     m_position -= m_right * velocity; break;
        case RIGHT:    m_position += m_right * velocity; break;
        case UP:       m_position += m_worldUp * velocity; break;
        case DOWN:     m_position -= m_worldUp * velocity; break;
    }
}

void Camera::processMouse(float xOffset, float yOffset) {
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    m_yaw += xOffset;
    m_pitch += yOffset;

    // Constrain pitch to avoid gimbal lock
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void Camera::processScroll(float yOffset) {
    m_fov -= yOffset * 2.0f;
    m_fov = std::clamp(m_fov, 20.0f, 120.0f);
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace voxel
