#pragma once
#include "camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace voxel {

class Input {
public:
    Input(GLFWwindow* window, Camera& camera);

    void processInput(float deltaTime);

    bool isKeyPressed(int key) const;
    bool wasKeyJustPressed(int key);

    // Mouse state
    double getMouseX() const { return m_mouseX; }
    double getMouseY() const { return m_mouseY; }
    bool isCursorCaptured() const { return m_cursorCaptured; }
    void toggleCursor();

private:
    GLFWwindow* m_window;
    Camera& m_camera;

    double m_mouseX = 0.0, m_mouseY = 0.0;
    double m_lastMouseX = 0.0, m_lastMouseY = 0.0;
    bool m_firstMouse = true;
    bool m_cursorCaptured = true;

    // Key state tracking for "just pressed" detection
    bool m_keyStates[GLFW_KEY_LAST + 1] = {};

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace voxel
