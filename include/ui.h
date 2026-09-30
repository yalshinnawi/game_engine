#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "shader.h"
#include "network.h"

namespace voxel {

struct UIVertex {
    glm::vec2 pos;
    glm::vec4 color;
};

class UIRenderer {
public:
    UIRenderer() = default;
    ~UIRenderer();

    void init(const std::string& shaderDir);

    void begin(int screenWidth, int screenHeight);
    void end();

    void drawRect(float x, float y, float w, float h, const glm::vec4& color);
    void drawRectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color);
    void drawCrosshair(int screenWidth, int screenHeight);

    // Pause menu rendering and hit testing
    void drawPauseMenu(int screenWidth, int screenHeight, bool isFlying,
                       NetworkMode netMode, int clientCount,
                       double mouseX, double mouseY, int& outHovered);

    int getClickedMenuButton(int screenWidth, int screenHeight, double mouseX, double mouseY) const;

private:
    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::vector<UIVertex> m_vertices;
    int m_screenWidth = 1280;
    int m_screenHeight = 720;

    void flush();
};

} // namespace voxel