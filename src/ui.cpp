#include "ui.h"
#include "font8x8.h"
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

    // Dynamic buffer with plenty of capacity for quads & text
    glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(UIVertex), nullptr, GL_DYNAMIC_DRAW);

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

void UIRenderer::drawChar(char c, float x, float y, float scale, const glm::vec4& color) {
    uint8_t uc = static_cast<uint8_t>(c);
    if (uc >= 128) uc = '?';

    const uint8_t* glyph = font8x8_basic[uc];
    for (int row = 0; row < 8; row++) {
        uint8_t byte = glyph[row];
        for (int col = 0; col < 8; col++) {
            if ((byte >> col) & 1) {
                drawRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void UIRenderer::drawText(const std::string& text, float x, float y, float scale, const glm::vec4& color, bool shadow) {
    if (shadow) {
        glm::vec4 shadowColor(0.0f, 0.0f, 0.0f, color.a * 0.8f);
        float curX = x + scale;
        float curY = y + scale;
        for (char c : text) {
            drawChar(c, curX, curY, scale, shadowColor);
            curX += 8.0f * scale;
        }
    }

    float curX = x;
    for (char c : text) {
        drawChar(c, curX, y, scale, color);
        curX += 8.0f * scale;
    }
}

void UIRenderer::drawTextCentered(const std::string& text, float centerX, float centerY, float scale, const glm::vec4& color, bool shadow) {
    float totalW = text.length() * 8.0f * scale;
    float totalH = 8.0f * scale;
    float startX = centerX - totalW * 0.5f;
    float startY = centerY - totalH * 0.5f;
    drawText(text, startX, startY, scale, color, shadow);
}

void UIRenderer::drawPauseMenu(int screenWidth, int screenHeight, bool isFlying,
                               NetworkMode netMode, ConnectionState connState, int clientCount,
                               uint32_t localId, const std::string& targetIP,
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

    // Header Title Text
    drawTextCentered("GAME PAUSED", cardX + cardW * 0.5f, cardY + 23.0f, 2.0f,
                     glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));

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
        std::string btnText;

        if (i == 0) {
            // Resume Game
            btnBg = hovered ? glm::vec4(0.20f, 0.35f, 0.25f, 1.0f) : glm::vec4(0.14f, 0.20f, 0.18f, 1.0f);
            btnBorder = hovered ? glm::vec4(0.40f, 0.85f, 0.50f, 1.0f) : glm::vec4(0.28f, 0.50f, 0.35f, 0.8f);
            accentColor = glm::vec4(0.35f, 0.85f, 0.45f, 1.0f);
            btnText = "RESUME GAME";
        } else if (i == 1) {
            // Toggle Game Mode
            btnBg = hovered ? glm::vec4(0.18f, 0.28f, 0.40f, 1.0f) : glm::vec4(0.14f, 0.18f, 0.26f, 1.0f);
            btnBorder = hovered ? glm::vec4(0.35f, 0.65f, 0.90f, 1.0f) : glm::vec4(0.22f, 0.40f, 0.60f, 0.8f);
            accentColor = isFlying ? glm::vec4(0.95f, 0.75f, 0.25f, 1.0f) : glm::vec4(0.25f, 0.75f, 0.95f, 1.0f);
            btnText = isFlying ? "MODE: CREATIVE (FLY)" : "MODE: SURVIVAL (WALK)";
        } else if (i == 2) {
            // Host Server
            bool isHost = (netMode == NetworkMode::SERVER);
            btnBg = hovered ? glm::vec4(0.35f, 0.25f, 0.40f, 1.0f) : glm::vec4(0.20f, 0.15f, 0.25f, 1.0f);
            btnBorder = isHost ? glm::vec4(0.70f, 0.40f, 0.95f, 1.0f) : (hovered ? glm::vec4(0.65f, 0.45f, 0.85f, 1.0f) : glm::vec4(0.40f, 0.30f, 0.55f, 0.8f));
            accentColor = isHost ? glm::vec4(0.40f, 0.95f, 0.40f, 1.0f) : glm::vec4(0.80f, 0.45f, 0.95f, 1.0f);
            btnText = isHost ? ("HOSTING (" + std::to_string(clientCount + 1) + " PLAYERS)") : "HOST SERVER (PORT 25565)";
        } else if (i == 3) {
            // Connect to Client
            if (connState == ConnectionState::CONNECTING) {
                btnBg = glm::vec4(0.35f, 0.30f, 0.15f, 1.0f);
                btnBorder = glm::vec4(0.95f, 0.85f, 0.30f, 1.0f);
                accentColor = glm::vec4(0.95f, 0.90f, 0.35f, 1.0f);
                btnText = "CONNECTING: " + targetIP + "...";
            } else if (connState == ConnectionState::CONNECTED && netMode == NetworkMode::CLIENT) {
                btnBg = glm::vec4(0.15f, 0.35f, 0.25f, 1.0f);
                btnBorder = glm::vec4(0.35f, 0.95f, 0.60f, 1.0f);
                accentColor = glm::vec4(0.40f, 0.95f, 0.65f, 1.0f);
                btnText = "CONNECTED: PLAYER #" + std::to_string(localId);
            } else if (connState == ConnectionState::FAILED) {
                btnBg = hovered ? glm::vec4(0.40f, 0.15f, 0.15f, 1.0f) : glm::vec4(0.28f, 0.12f, 0.14f, 1.0f);
                btnBorder = glm::vec4(0.95f, 0.35f, 0.35f, 1.0f);
                accentColor = glm::vec4(0.95f, 0.45f, 0.45f, 1.0f);
                btnText = "FAILED! RETRY: " + targetIP;
            } else {
                btnBg = hovered ? glm::vec4(0.25f, 0.35f, 0.35f, 1.0f) : glm::vec4(0.15f, 0.22f, 0.22f, 1.0f);
                btnBorder = (hovered ? glm::vec4(0.45f, 0.80f, 0.75f, 1.0f) : glm::vec4(0.30f, 0.50f, 0.48f, 0.8f));
                accentColor = glm::vec4(0.40f, 0.75f, 0.70f, 1.0f);
                btnText = "CONNECT: " + targetIP;
            }
        } else {
            // Quit
            btnBg = hovered ? glm::vec4(0.38f, 0.18f, 0.20f, 1.0f) : glm::vec4(0.22f, 0.14f, 0.16f, 1.0f);
            btnBorder = hovered ? glm::vec4(0.95f, 0.40f, 0.45f, 1.0f) : glm::vec4(0.55f, 0.28f, 0.32f, 0.8f);
            accentColor = glm::vec4(0.95f, 0.35f, 0.40f, 1.0f);
            btnText = "QUIT TO DESKTOP";
        }

        drawRect(btnX, by, btnW, btnH, btnBg);
        drawRectOutline(btnX, by, btnW, btnH, hovered ? 2.5f : 1.5f, btnBorder);
        drawRect(btnX + 4.0f, by + 6.0f, 6.0f, btnH - 12.0f, accentColor);

        // Draw Button Text
        glm::vec4 txtColor = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.85f, 0.88f, 0.92f, 0.9f);
        drawTextCentered(btnText, btnX + btnW * 0.5f, by + btnH * 0.5f, 1.75f, txtColor);
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