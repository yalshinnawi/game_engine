#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "shader.h"
#include "camera.h"
#include "network.h"

namespace voxel {

struct AvatarVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
};

class AvatarRenderer {
public:
    AvatarRenderer() = default;
    ~AvatarRenderer();

    void init(const std::string& shaderDir);
    void render(const Camera& camera, float aspectRatio,
                const std::vector<RemotePlayer>& players,
                const glm::vec3& skyColor, float fogDistance);

private:
    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_vertexCount = 0;

    void buildAvatarMesh();
    void addBox(std::vector<AvatarVertex>& vertices,
                const glm::vec3& min, const glm::vec3& max,
                const glm::vec3& color);
};

} // namespace voxel