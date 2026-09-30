#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace voxel {

struct WindowData {
    int width = 1280;
    int height = 720;
    std::function<void(int, int)> resizeCallback;
    std::function<void(double, double)> cursorPosCallback;
    std::function<void(int, int, int)> mouseButtonCallback;
    std::function<void(double, double)> scrollCallback;
    std::function<void(int, int, int, int)> keyCallback;
};

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getHandle() const { return m_window; }
    int getWidth() const { return m_data.width; }
    int getHeight() const { return m_data.height; }
    float getAspectRatio() const {
        if (m_data.height <= 0) return 1.0f;
        return static_cast<float>(m_data.width) / static_cast<float>(m_data.height);
    }

    void setResizeCallback(std::function<void(int, int)> callback);
    void setCursorPosCallback(std::function<void(double, double)> callback);
    void setMouseButtonCallback(std::function<void(int, int, int)> callback);
    void setScrollCallback(std::function<void(double, double)> callback);
    void setKeyCallback(std::function<void(int, int, int, int)> callback);

    void setCursorMode(int mode);

private:
    GLFWwindow* m_window = nullptr;
    WindowData m_data;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

} // namespace voxel