#include "ui.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace voxel {

UIRenderer::~UIRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void UIRenderer::init(const std::string& shaderDir) {
    m_shader = Shader(shaderDir + "/ui.vert", shaderDir + "/ui.frag");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Dynamic buffer
    glBufferData(GL_ARRAY_BUFFER, 2048 * sizeof(UIVertex), nullptr, GL_DYNAMIC_DRAW);

    // aPos (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)offsetof(UIVertex, pos));
    glEnableVertexAttribArray(0);

    // aColor (location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)offsetof(UIVertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void UIRenderer::begin(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_vertices.clear();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    m_shader.use();
    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(screenWidth),
                                static_cast<float>(screenHeight), 0.0f,
                                -1.0f, 1.0f);
    m_shader.setMat4("u_projection", proj);
}

void UIRenderer::end() {
    flush();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void UIRenderer::flush() {
    if (m_vertices.empty()) return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(UIVertex), m_vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);

    m_vertices.clear();
}

void UIRenderer::drawRect(float x, float y, float w, float h, const glm::vec4& color) {
    UIVertex v0{{x,     y},     color};
    UIVertex v1{{x + w, y},     color};
    UIVertex v2{{x + w, y + h}, color};
    UIVertex v3{{x,     y + h}, color};

    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);

    m_vertices.push_back(v0);
    m_vertices.push_back(v2);
    m_vertices.push_back(v3);
}

void UIRenderer::drawRectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color) {
    drawRect(x, y, w, thickness, color); // Top
    drawRect(x, y + h - thickness, w, thickness, color); // Bottom
    drawRect(x, y, thickness, h, color); // Left
    drawRect(x + w - thickness, y, thickness, h, color); // Right
}

void UIRenderer::drawCrosshair(int screenWidth, int screenHeight) {
    float cx = screenWidth * 0.5f;
    float cy = screenHeight * 0.5f;
    float size = 8.0f;
    float thick = 2.0f;

    glm::vec4 chColor(1.0f, 1.0f, 1.0f, 0.85f);
    glm::vec4 shadowColor(0.0f, 0.0f, 0.0f, 0.5f);

    // Shadow
    drawRect(cx - size - 1.0f, cy - thick * 0.5f - 1.0f, size * 2.0f + 2.0f, thick + 2.0f, shadowColor);
    drawRect(cx - thick * 0.5f - 1.0f, cy - size - 1.0f, thick + 2.0f, size * 2.0f + 2.0f, shadowColor);

    // White crosshair
    drawRect(cx - size, cy - thick * 0.5f, size * 2.0f, thick, chColor);
    drawRect(cx - thick * 0.5f, cy - size, thick, size * 2.0f, chColor);
}

void UIRenderer::drawPauseMenu(int screenWidth, int screenHeight, bool isFlying,
                               NetworkMode netMode, int clientCount,
                               double mouseX, double mouseY, int& outHovered) {
    // Dim background
    drawRect(0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight),
             glm::vec4(0.04f, 0.06f, 0.10f, 0.72f));

    float cardW = 420.0f;
    float cardH = 380.0f;
    float cardX = (screenWidth - cardW) * 0.5f;
    float cardY = (screenHeight - cardH) * 0.5f;

    // Card background
    drawRect(cardX, cardY, cardW, cardH, glm::vec4(0.11f, 0.13f, 0.17f, 0.96f));
    drawRectOutline(cardX, cardY, cardW, cardH, 2.0f, glm::vec4(0.32f, 0.40f, 0.52f, 1.0f));

    // Header bar
    drawRect(cardX, cardY, cardW, 46.0f, glm::vec4(0.16f, 0.20f, 0.28f, 1.0f));
    drawRect(cardX, cardY + 44.0f, cardW, 2.0f, glm::vec4(0.25f, 0.65f, 0.95f, 1.0f));

    // Button geometry
    float btnW = 360.0f;
    float btnH = 44.0f;
    float btnX = (screenWidth - btnW) * 0.5f;
    float startY = cardY + 60.0f;
    float spacing = 54.0f;

    outHovered = -1;

    for (int i = 0; i < 5; i++) {
        float by = startY + i * spacing;
        bool hovered = (mouseX >= btnX && mouseX <= btnX + btnW &&
                        mouseY >= by   && mouseY <= by + btnH);

        if (hovered) outHovered = i;

        glm::vec4 btnBg;
        glm::vec4 btnBorder;
        glm::vec4 accentColor;

        if (i == 0) {
            // Resume Game
            btnBg = hovered ? glm::vec4(0.20f, 0.35f, 0.25f, 1.0f) : glm::vec4(0.14f, 0.20f, 0.18f, 1.0f);
            btnBorder = hovered ? glm::vec4(0.40f, 0.85f, 0.50f, 1.0f) : glm::vec4(0.28f, 0.50f, 0.35f, 0.8f);
            accentColor = glm::vec4(0.35f, 0.85f, 0.45f, 1.0f);
        } else if (i == 1) {
            // Toggle Game Mode
            btnBg = hovered ? glm::vec4(0.18f, 0.28f, 0.40f, 1.0f) : glm::vec4(0.14f, 0.18f, 0.26f, 1.0f);
            btnBorder = hovered ? glm::vec4(0.35f, 0.65f, 0.90f, 1.0f) : glm::vec4(0.22f, 0.40f, 0.60f, 0.8f);
            accentColor = isFlying ? glm::vec4(0.95f, 0.75f, 0.25f, 1.0f) : glm::vec4(0.25f, 0.75f, 0.95f, 1.0f);
        } else if (i == 2) {
            // Host Server
            bool isHost = (netMode == NetworkMode::SERVER);
            btnBg = hovered ? glm::vec4(0.35f, 0.25f, 0.40f, 1.0f) : glm::vec4(0.20f, 0.15f, 0.25f, 1.0f);
            btnBorder = isHost ? glm::vec4(0.70f, 0.40f, 0.95f, 1.0f) : (hovered ? glm::vec4(0.65f, 0.45f, 0.85f, 1.0f) : glm::vec4(0.40f, 0.30f, 0.55f, 0.8f));
            accentColor = isHost ? glm::vec4(0.40f, 0.95f, 0.40f, 1.0f) : glm::vec4(0.80f, 0.45f, 0.95f, 1.0f);
        } else if (i == 3) {
            // Connect to Client
            bool isClient = (netMode == NetworkMode::CLIENT);
            btnBg = hovered ? glm::vec4(0.25f, 0.35f, 0.35f, 1.0f) : glm::vec4(0.15f, 0.22f, 0.22f, 1.0f);
            btnBorder = isClient ? glm::vec4(0.35f, 0.90f, 0.85f, 1.0f) : (hovered ? glm::vec4(0.45f, 0.80f, 0.75f, 1.0f) : glm::vec4(0.30f, 0.50f, 0.48f, 0.8f));
            accentColor = isClient ? glm::vec4(0.35f, 0.95f, 0.85f, 1.0f) : glm::vec4(0.40f, 0.75f, 0.70f, 1.0f);
        } else {
            // Quit
            btnBg = hovered ? glm::vec4(0.38f, 0.18f, 0.20f, 1.0f) : glm::vec4(0.22f, 0.14f, 0.16f, 1.0f);
            btnBorder = hovered ? glm::vec4(0.95f, 0.40f, 0.45f, 1.0f) : glm::vec4(0.55f, 0.28f, 0.32f, 0.8f);
            accentColor = glm::vec4(0.95f, 0.35f, 0.40f, 1.0f);
        }

        drawRect(btnX, by, btnW, btnH, btnBg);
        drawRectOutline(btnX, by, btnW, btnH, hovered ? 2.5f : 1.5f, btnBorder);
        drawRect(btnX + 4.0f, by + 6.0f, 6.0f, btnH - 12.0f, accentColor);
    }
}

int UIRenderer::getClickedMenuButton(int screenWidth, int screenHeight, double mouseX, double mouseY) const {
    float cardW = 420.0f;
    float cardH = 380.0f;
    float cardY = (screenHeight - cardH) * 0.5f;

    float btnW = 360.0f;
    float btnH = 44.0f;
    float btnX = (screenWidth - btnW) * 0.5f;
    float startY = cardY + 60.0f;
    float spacing = 54.0f;

    for (int i = 0; i < 5; i++) {
        float by = startY + i * spacing;
        if (mouseX >= btnX && mouseX <= btnX + btnW &&
            mouseY >= by   && mouseY <= by + btnH) {
            return i;
        }
    }
    return -1;
}

} // namespace voxel