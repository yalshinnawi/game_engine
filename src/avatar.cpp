#include "avatar.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace voxel {

AvatarRenderer::~AvatarRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void AvatarRenderer::init(const std::string& shaderDir) {
    m_shader = Shader(shaderDir + "/avatar.vert", shaderDir + "/avatar.frag");
    buildAvatarMesh();
}

void AvatarRenderer::addBox(std::vector<AvatarVertex>& vertices,
                            const glm::vec3& min, const glm::vec3& max,
                            const glm::vec3& color) {
    float x0 = min.x, x1 = max.x;
    float y0 = min.y, y1 = max.y;
    float z0 = min.z, z1 = max.z;

    auto addQuad = [&](const glm::vec3& p0, const glm::vec3& p1,
                       const glm::vec3& p2, const glm::vec3& p3,
                       const glm::vec3& normal) {
        vertices.push_back({p0, normal, color});
        vertices.push_back({p1, normal, color});
        vertices.push_back({p2, normal, color});

        vertices.push_back({p0, normal, color});
        vertices.push_back({p2, normal, color});
        vertices.push_back({p3, normal, color});
    };

    // Top (+Y)
    addQuad({x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, {0, 1, 0});
    // Bottom (-Y)
    addQuad({x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {0, -1, 0});
    // Front (+Z)
    addQuad({x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1});
    // Back (-Z)
    addQuad({x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0, 0, -1});
    // Right (+X)
    addQuad({x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0});
    // Left (-X)
    addQuad({x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0});
}

void AvatarRenderer::buildAvatarMesh() {
    std::vector<AvatarVertex> vertices;

    glm::vec3 skinColor(0.88f, 0.72f, 0.58f);
    glm::vec3 shirtColor(0.12f, 0.65f, 0.75f);
    glm::vec3 pantsColor(0.20f, 0.30f, 0.70f);
    glm::vec3 hairColor(0.40f, 0.25f, 0.15f);

    // Head (0.4m cube)
    addBox(vertices, {-0.20f, 1.40f, -0.20f}, {0.20f, 1.80f, 0.20f}, skinColor);

    // Hair cap on head top & back
    addBox(vertices, {-0.21f, 1.70f, -0.21f}, {0.21f, 1.82f, 0.21f}, hairColor);
    addBox(vertices, {-0.21f, 1.50f, -0.21f}, {0.21f, 1.70f, -0.10f}, hairColor);

    // Torso (0.4m x 0.7m x 0.2m)
    addBox(vertices, {-0.20f, 0.70f, -0.10f}, {0.20f, 1.40f, 0.10f}, shirtColor);

    // Left Arm
    addBox(vertices, {-0.35f, 0.70f, -0.10f}, {-0.20f, 1.40f, 0.10f}, shirtColor);
    addBox(vertices, {-0.35f, 0.55f, -0.10f}, {-0.20f, 0.70f, 0.10f}, skinColor);

    // Right Arm
    addBox(vertices, {0.20f, 0.70f, -0.10f}, {0.35f, 1.40f, 0.10f}, shirtColor);
    addBox(vertices, {0.20f, 0.55f, -0.10f}, {0.35f, 0.70f, 0.10f}, skinColor);

    // Left Leg
    addBox(vertices, {-0.19f, 0.00f, -0.10f}, {-0.02f, 0.70f, 0.10f}, pantsColor);

    // Right Leg
    addBox(vertices, {0.02f, 0.00f, -0.10f}, {0.19f, 0.70f, 0.10f}, pantsColor);

    m_vertexCount = static_cast<int>(vertices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(AvatarVertex), vertices.data(), GL_STATIC_DRAW);

    // aPos (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AvatarVertex), (void*)offsetof(AvatarVertex, pos));
    glEnableVertexAttribArray(0);

    // aNormal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AvatarVertex), (void*)offsetof(AvatarVertex, normal));
    glEnableVertexAttribArray(1);

    // aColor (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AvatarVertex), (void*)offsetof(AvatarVertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void AvatarRenderer::render(const Camera& camera, float aspectRatio,
                            const std::vector<RemotePlayer>& players,
                            const glm::vec3& skyColor, float fogDistance) {
    if (players.empty() || m_vertexCount == 0) return;

    m_shader.use();
    m_shader.setMat4("u_view", camera.getViewMatrix());
    m_shader.setMat4("u_projection", camera.getProjectionMatrix(aspectRatio));
    m_shader.setVec3("u_cameraPos", camera.getPosition());
    m_shader.setVec3("u_skyColor", skyColor);
    m_shader.setFloat("u_fogDistance", fogDistance);

    glBindVertexArray(m_vao);

    for (const auto& player : players) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), player.position);
        model = glm::rotate(model, glm::radians(-player.yaw - 90.0f), glm::vec3(0, 1, 0));
        m_shader.setMat4("u_model", model);

        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    }

    glBindVertexArray(0);
}

} // namespace voxel