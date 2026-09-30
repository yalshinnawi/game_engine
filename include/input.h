#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "camera.h"

namespace voxel {

class Window;

class Input {
public:
    Input(Window& window, Camera& camera);

    void update();

    bool isKeyPressed(int key) const;
    bool wasKeyJustPressed(int key);

    bool isMouseButtonPressed(int button) const;
    bool wasMouseButtonJustPressed(int button);

    double getMouseX() const { return m_mouseX; }
    double getMouseY() const { return m_mouseY; }

    bool isCursorCaptured() const { return m_cursorCaptured; }
    void setCursorCaptured(bool captured);
    void toggleCursor();

    int getScrollDelta() {
        int delta = m_scrollDelta;
        m_scrollDelta = 0;
        return delta;
    }

private:
    Window& m_window;
    Camera& m_camera;

    double m_mouseX = 0.0, m_mouseY = 0.0;
    double m_lastMouseX = 0.0, m_lastMouseY = 0.0;
    bool m_firstMouse = true;
    bool m_cursorCaptured = true;

    int m_scrollDelta = 0;

    bool m_keyStates[GLFW_KEY_LAST + 1] = {};
    bool m_keyJustPressed[GLFW_KEY_LAST + 1] = {};

    bool m_mouseButtonStates[GLFW_MOUSE_BUTTON_LAST + 1] = {};
    bool m_mouseButtonJustPressed[GLFW_MOUSE_BUTTON_LAST + 1] = {};

    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);
    void onKey(int key, int scancode, int action, int mods);
};

} // namespace voxel