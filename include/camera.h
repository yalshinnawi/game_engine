#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace voxel {

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 64.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void processKeyboard(int direction, float deltaTime);
    void processMouse(float xOffset, float yOffset);
    void processScroll(float yOffset);

    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    glm::vec3 getFront() const { return m_front; }
    float getFov() const { return m_fov; }

    // Movement directions
    enum Direction { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

    // Speed settings
    float movementSpeed = 10.0f;
    float mouseSensitivity = 0.1f;

private:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;
    float m_fov = 70.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 500.0f;

    void updateCameraVectors();
};

} // namespace voxel