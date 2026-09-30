#include "window.h"
#include <iostream>
#include <stdexcept>

namespace voxel {

static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "[GLFW Error " << error << "] " << description << std::endl;
}

Window::Window(int width, int height, const std::string& title) {
    m_data.width = width;
    m_data.height = height;

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwSwapInterval(1);

    // Set user pointer to WindowData
    glfwSetWindowUserPointer(m_window, &m_data);

    // Register callbacks
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetKeyCallback(m_window, keyCallback);

    std::cout << "========================================" << std::endl;
    std::cout << "  VoxelEngine Initialized" << std::endl;
    std::cout << "  OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "  GPU:    " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "========================================" << std::endl;
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::setResizeCallback(std::function<void(int, int)> callback) {
    m_data.resizeCallback = std::move(callback);
}

void Window::setCursorPosCallback(std::function<void(double, double)> callback) {
    m_data.cursorPosCallback = std::move(callback);
}

void Window::setMouseButtonCallback(std::function<void(int, int, int)> callback) {
    m_data.mouseButtonCallback = std::move(callback);
}

void Window::setScrollCallback(std::function<void(double, double)> callback) {
    m_data.scrollCallback = std::move(callback);
}

void Window::setKeyCallback(std::function<void(int, int, int, int)> callback) {
    m_data.keyCallback = std::move(callback);
}

void Window::setCursorMode(int mode) {
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0) return;
    auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
    if (!data) return;

    data->width = width;
    data->height = height;
    glViewport(0, 0, width, height);

    if (data->resizeCallback) {
        data->resizeCallback(width, height);
    }
}

void Window::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
    if (data && data->cursorPosCallback) {
        data->cursorPosCallback(xpos, ypos);
    }
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
    if (data && data->mouseButtonCallback) {
        data->mouseButtonCallback(button, action, mods);
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
    if (data && data->scrollCallback) {
        data->scrollCallback(xoffset, yoffset);
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
    if (data && data->keyCallback) {
        data->keyCallback(key, scancode, action, mods);
    }
}

} // namespace voxel