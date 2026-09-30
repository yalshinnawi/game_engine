#pragma once
#include <glm/glm.hpp>
#include "camera.h"
#include "world.h"
#include "input.h"

namespace voxel {

class Player {
public:
    Player(const glm::vec3& spawnPos);

    void update(float deltaTime, World& world, Camera& camera, const Input& input);

    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getEyePosition() const { return m_position + glm::vec3(0.0f, m_eyeHeight, 0.0f); }
    void setPosition(const glm::vec3& pos) { m_position = pos; m_velocity = glm::vec3(0.0f); }

    bool isFlying() const { return m_isFlying; }
    void setFlying(bool flying) { m_isFlying = flying; m_velocity = glm::vec3(0.0f); }
    void toggleFlying() { setFlying(!m_isFlying); }

    bool isGrounded() const { return m_isGrounded; }

    BlockType getSelectedBlock() const { return m_selectedBlock; }
    void setSelectedBlock(BlockType type) { m_selectedBlock = type; }
    void cycleSelectedBlock(int dir);

    // Collision box dimensions
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    float getEyeHeight() const { return m_eyeHeight; }

private:
    glm::vec3 m_position; // Feet position
    glm::vec3 m_velocity = glm::vec3(0.0f);

    bool m_isGrounded = false;
    bool m_isFlying = false;

    float m_eyeHeight = 1.62f;
    float m_width = 0.5f;   // half-width = 0.25f
    float m_height = 1.8f;

    float m_walkSpeed = 4.3f;
    float m_sprintSpeed = 7.5f;
    float m_flySpeed = 16.0f;
    float m_jumpVelocity = 8.2f;
    float m_gravity = -24.0f;

    BlockType m_selectedBlock = BlockType::STONE;

    bool checkCollision(const glm::vec3& pos, const World& world) const;
};

} // namespace voxel