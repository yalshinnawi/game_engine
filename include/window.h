#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace voxel {

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
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

    void setResizeCallback(std::function<void(int, int)> callback);
    void setCursorMode(int mode);

private:
    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    std::string m_title;
    std::function<void(int, int)> m_resizeCallback;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace voxel
