#pragma once
#include "shader.h"
#include "camera.h"
#include "world.h"
#include <glm/glm.hpp>

namespace voxel {

class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    void init(const std::string& shaderDir);
    void beginFrame(const Camera& camera, float aspectRatio);
    void renderWorld(const World& world);
    void endFrame();

    void setWireframe(bool enabled);
    bool isWireframe() const { return m_wireframe; }

    // Sky color (for clearing and fog)
    void setSkyColor(const glm::vec3& color) { m_skyColor = color; }
    glm::vec3 getSkyColor() const { return m_skyColor; }

    // Fog
    void setFogDistance(float distance) { m_fogDistance = distance; }

    Shader& getChunkShader() { return m_chunkShader; }

private:
    Shader m_chunkShader;
    bool m_wireframe = false;
    glm::vec3 m_skyColor = glm::vec3(0.53f, 0.81f, 0.92f); // Light blue sky
    float m_fogDistance = 200.0f;
};

} // namespace voxel
