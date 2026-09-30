#include "input.h"
#include <iostream>

namespace voxel {

Input::Input(GLFWwindow* window, Camera& camera)
    : m_window(window), m_camera(camera) {

    // Store this pointer for static callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize mouse position
    glfwGetCursorPos(window, &m_lastMouseX, &m_lastMouseY);
}

void Input::processInput(float deltaTime) {
    // Escape to release cursor
    if (wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
        toggleCursor();
    }

    // Movement (only when cursor captured)
    if (m_cursorCaptured) {
        if (isKeyPressed(GLFW_KEY_W)) m_camera.processKeyboard(Camera::FORWARD, deltaTime);
        if (isKeyPressed(GLFW_KEY_S)) m_camera.processKeyboard(Camera::BACKWARD, deltaTime);
        if (isKeyPressed(GLFW_KEY_A)) m_camera.processKeyboard(Camera::LEFT, deltaTime);
        if (isKeyPressed(GLFW_KEY_D)) m_camera.processKeyboard(Camera::RIGHT, deltaTime);
        if (isKeyPressed(GLFW_KEY_SPACE)) m_camera.processKeyboard(Camera::UP, deltaTime);
        if (isKeyPressed(GLFW_KEY_LEFT_SHIFT)) m_camera.processKeyboard(Camera::DOWN, deltaTime);
    }

    // Sprint
    if (isKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
        m_camera.movementSpeed = 30.0f;
    } else {
        m_camera.movementSpeed = 10.0f;
    }

    // Close window
    if (isKeyPressed(GLFW_KEY_Q)) {
        glfwSetWindowShouldClose(m_window, true);
    }
}

bool Input::isKeyPressed(int key) const {
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool Input::wasKeyJustPressed(int key) {
    bool pressed = glfwGetKey(m_window, key) == GLFW_PRESS;
    bool justPressed = pressed && !m_keyStates[key];
    m_keyStates[key] = pressed;
    return justPressed;
}

void Input::toggleCursor() {
    m_cursorCaptured = !m_cursorCaptured;
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     m_cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    m_firstMouse = true;
}

void Input::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_cursorCaptured) return;

    self->m_mouseX = xpos;
    self->m_mouseY = ypos;

    if (self->m_firstMouse) {
        self->m_lastMouseX = xpos;
        self->m_lastMouseY = ypos;
        self->m_firstMouse = false;
    }

    double xOffset = xpos - self->m_lastMouseX;
    double yOffset = self->m_lastMouseY - ypos; // Reversed: y goes bottom-to-top

    self->m_lastMouseX = xpos;
    self->m_lastMouseY = ypos;

    self->m_camera.processMouse(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void Input::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_camera.processScroll(static_cast<float>(yoffset));
}

} // namespace voxel
