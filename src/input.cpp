#include "input.h"
#include "window.h"
#include <iostream>

namespace voxel {

Input::Input(Window& window, Camera& camera)
    : m_window(window), m_camera(camera) {

    // Register callbacks with Window
    window.setCursorPosCallback([this](double x, double y) { onMouseMove(x, y); });
    window.setMouseButtonCallback([this](int b, int a, int m) { onMouseButton(b, a, m); });
    window.setScrollCallback([this](double x, double y) { onScroll(x, y); });
    window.setKeyCallback([this](int k, int sc, int a, int m) { onKey(k, sc, a, m); });

    // Capture cursor initially
    setCursorCaptured(true);
}

void Input::update() {
    // Clear just-pressed flags
    for (int i = 0; i <= GLFW_KEY_LAST; i++) {
        m_keyJustPressed[i] = false;
    }
    for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
        m_mouseButtonJustPressed[i] = false;
    }
}

bool Input::isKeyPressed(int key) const {
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return m_keyStates[key];
}

bool Input::wasKeyJustPressed(int key) {
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return m_keyJustPressed[key];
}

bool Input::isMouseButtonPressed(int button) const {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return m_mouseButtonStates[button];
}

bool Input::wasMouseButtonJustPressed(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return m_mouseButtonJustPressed[button];
}

void Input::setCursorCaptured(bool captured) {
    m_cursorCaptured = captured;
    m_window.setCursorMode(captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    m_firstMouse = true;
}

void Input::toggleCursor() {
    setCursorCaptured(!m_cursorCaptured);
}

void Input::onMouseMove(double xpos, double ypos) {
    m_mouseX = xpos;
    m_mouseY = ypos;

    if (!m_cursorCaptured) return;

    if (m_firstMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
        return;
    }

    double xOffset = xpos - m_lastMouseX;
    double yOffset = m_lastMouseY - ypos;

    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    m_camera.processMouse(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void Input::onMouseButton(int button, int action, int /*mods*/) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return;
    if (action == GLFW_PRESS) {
        m_mouseButtonStates[button] = true;
        m_mouseButtonJustPressed[button] = true;
    } else if (action == GLFW_RELEASE) {
        m_mouseButtonStates[button] = false;
    }
}

void Input::onScroll(double /*xoffset*/, double yoffset) {
    m_scrollDelta += static_cast<int>(yoffset);
    if (m_cursorCaptured) {
        m_camera.processScroll(static_cast<float>(yoffset));
    }
}

void Input::onKey(int key, int /*scancode*/, int action, int /*mods*/) {
    if (key < 0 || key > GLFW_KEY_LAST) return;
    if (action == GLFW_PRESS) {
        m_keyStates[key] = true;
        m_keyJustPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        m_keyStates[key] = false;
    }
}

} // namespace voxel