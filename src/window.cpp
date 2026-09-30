#include "window.h"
#include <iostream>
#include <stdexcept>

namespace voxel {

static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "[GLFW Error " << error << "] " << description << std::endl;
}

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height), m_title(title) {

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Request OpenGL 4.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    // Load OpenGL functions via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Enable VSync
    glfwSwapInterval(1);

    // Store this pointer for callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Print GPU info
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
    m_resizeCallback = std::move(callback);
}

void Window::setCursorMode(int mode) {
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_width = width;
    self->m_height = height;
    glViewport(0, 0, width, height);
    if (self->m_resizeCallback) {
        self->m_resizeCallback(width, height);
    }
}

} // namespace voxel
