#include "player.h"
#include <cmath>
#include <algorithm>

namespace voxel {

Player::Player(const glm::vec3& spawnPos)
    : m_position(spawnPos) {}

bool Player::checkCollision(const glm::vec3& pos, const World& world) const {
    float halfW = m_width * 0.5f;
    glm::vec3 boxMin = pos - glm::vec3(halfW, 0.0f, halfW);
    glm::vec3 boxMax = pos + glm::vec3(halfW, m_height, halfW);

    int minX = static_cast<int>(std::floor(boxMin.x));
    int maxX = static_cast<int>(std::floor(boxMax.x));
    int minY = static_cast<int>(std::floor(boxMin.y));
    int maxY = static_cast<int>(std::floor(boxMax.y));
    int minZ = static_cast<int>(std::floor(boxMin.z));
    int maxZ = static_cast<int>(std::floor(boxMax.z));

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (world.isSolidAt(x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Player::cycleSelectedBlock(int dir) {
    int cur = static_cast<int>(m_selectedBlock);
    cur += dir;
    if (cur <= 0) cur = static_cast<int>(BlockType::SNOW);
    if (cur > static_cast<int>(BlockType::SNOW)) cur = 1;
    m_selectedBlock = static_cast<BlockType>(cur);
}

void Player::update(float deltaTime, World& world, Camera& camera, const Input& input) {
    if (m_isFlying) {
        // 6-Axis Creative Flying
        glm::vec3 front = camera.getFront();
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 wishDir(0.0f);

        if (input.isKeyPressed(GLFW_KEY_W)) wishDir += front;
        if (input.isKeyPressed(GLFW_KEY_S)) wishDir -= front;
        if (input.isKeyPressed(GLFW_KEY_A)) wishDir -= right;
        if (input.isKeyPressed(GLFW_KEY_D)) wishDir += right;
        if (input.isKeyPressed(GLFW_KEY_SPACE)) wishDir += glm::vec3(0, 1, 0);
        if (input.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) wishDir -= glm::vec3(0, 1, 0);

        if (glm::length(wishDir) > 0.001f) {
            wishDir = glm::normalize(wishDir);
        }

        float speed = input.isKeyPressed(GLFW_KEY_LEFT_CONTROL) ? m_flySpeed * 2.0f : m_flySpeed;
        m_position += wishDir * speed * deltaTime;
        m_velocity = glm::vec3(0.0f);

        camera.setPosition(getEyePosition());
        return;
    }

    // Survival Walking Mode with Gravity & Voxel Collision
    glm::vec3 front = camera.getFront();
    glm::vec3 forward = glm::vec3(front.x, 0.0f, front.z);
    if (glm::length(forward) > 0.001f) {
        forward = glm::normalize(forward);
    } else {
        forward = glm::vec3(0, 0, -1);
    }
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

    glm::vec3 wishDir(0.0f);
    if (input.isKeyPressed(GLFW_KEY_W)) wishDir += forward;
    if (input.isKeyPressed(GLFW_KEY_S)) wishDir -= forward;
    if (input.isKeyPressed(GLFW_KEY_A)) wishDir -= right;
    if (input.isKeyPressed(GLFW_KEY_D)) wishDir += right;

    if (glm::length(wishDir) > 0.001f) {
        wishDir = glm::normalize(wishDir);
    }

    float speed = input.isKeyPressed(GLFW_KEY_LEFT_CONTROL) ? m_sprintSpeed : m_walkSpeed;
    m_velocity.x = wishDir.x * speed;
    m_velocity.z = wishDir.z * speed;

    // Gravity
    m_velocity.y += m_gravity * deltaTime;
    if (m_velocity.y < -35.0f) {
        m_velocity.y = -35.0f;
    }

    // Jump
    if (input.isKeyPressed(GLFW_KEY_SPACE) && m_isGrounded) {
        m_velocity.y = m_jumpVelocity;
        m_isGrounded = false;
    }

    // Move & Collide Axis-by-Axis
    // 1. Y Axis
    m_position.y += m_velocity.y * deltaTime;
    if (checkCollision(m_position, world)) {
        if (m_velocity.y < 0.0f) {
            // Landed on ground
            m_position.y = std::ceil(m_position.y - 0.001f);
            m_velocity.y = 0.0f;
            m_isGrounded = true;
        } else if (m_velocity.y > 0.0f) {
            // Hit ceiling
            m_position.y = std::floor(m_position.y + m_height) - m_height - 0.001f;
            m_velocity.y = 0.0f;
        }
    } else {
        // Ground check
        if (m_velocity.y <= 0.0f && checkCollision(m_position - glm::vec3(0.0f, 0.05f, 0.0f), world)) {
            m_isGrounded = true;
        } else {
            m_isGrounded = false;
        }
    }

    // 2. X Axis
    m_position.x += m_velocity.x * deltaTime;
    if (checkCollision(m_position, world)) {
        m_position.x -= m_velocity.x * deltaTime;
        m_velocity.x = 0.0f;
    }

    // 3. Z Axis
    m_position.z += m_velocity.z * deltaTime;
    if (checkCollision(m_position, world)) {
        m_position.z -= m_velocity.z * deltaTime;
        m_velocity.z = 0.0f;
    }

    camera.setPosition(getEyePosition());
}

} // namespace voxel