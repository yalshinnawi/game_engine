#include "renderer.h"
#include <iostream>

namespace voxel {

Renderer::Renderer() {}

void Renderer::init(const std::string& shaderDir) {
    // Load chunk shader
    m_chunkShader = Shader(shaderDir + "/chunk.vert", shaderDir + "/chunk.frag");

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

    std::cout << "[Renderer] Initialized" << std::endl;
}

void Renderer::beginFrame(const Camera& camera, float aspectRatio) {
    glClearColor(m_skyColor.r, m_skyColor.g, m_skyColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_chunkShader.use();
    m_chunkShader.setMat4("u_view", camera.getViewMatrix());
    m_chunkShader.setMat4("u_projection", camera.getProjectionMatrix(aspectRatio));
    m_chunkShader.setVec3("u_cameraPos", camera.getPosition());
    m_chunkShader.setVec3("u_skyColor", m_skyColor);
    m_chunkShader.setFloat("u_fogDistance", m_fogDistance);
}

void Renderer::renderWorld(const World& world) {
    world.render();
}

void Renderer::endFrame() {
    // Post-processing would go here
}

void Renderer::setWireframe(bool enabled) {
    m_wireframe = enabled;
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

} // namespace voxel
