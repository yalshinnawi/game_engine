#include "shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace voxel {

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
    : m_vertexPath(vertexPath), m_fragmentPath(fragmentPath) {

    std::string vertexSrc = loadFile(vertexPath);
    std::string fragmentSrc = loadFile(fragmentPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    linkProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() {
    cleanup();
}

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program),
      m_vertexPath(std::move(other.m_vertexPath)),
      m_fragmentPath(std::move(other.m_fragmentPath)),
      m_uniformCache(std::move(other.m_uniformCache)) {
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_program = other.m_program;
        m_vertexPath = std::move(other.m_vertexPath);
        m_fragmentPath = std::move(other.m_fragmentPath);
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_program = 0;
    }
    return *this;
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2f(getUniformLocation(name), value.x, value.y);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::reload() {
    std::cout << "[Shader] Hot-reloading shaders..." << std::endl;

    std::string vertexSrc = loadFile(m_vertexPath);
    std::string fragmentSrc = loadFile(m_fragmentPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, vertexShader);
    glAttachShader(newProgram, fragmentShader);
    glLinkProgram(newProgram);

    GLint success;
    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);

    if (success) {
        cleanup();
        m_program = newProgram;
        m_uniformCache.clear();
        std::cout << "[Shader] Reload successful!" << std::endl;
    } else {
        char infoLog[512];
        glGetProgramInfoLog(newProgram, 512, nullptr, infoLog);
        std::cerr << "[Shader] Reload failed: " << infoLog << std::endl;
        glDeleteProgram(newProgram);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }
    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniformCache[name] = location;
    return location;
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::string shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "[Shader] " << shaderType << " compilation failed:\n" << infoLog << std::endl;
    }
    return shader;
}

std::string Shader::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Shader] Failed to open: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Shader::linkProgram(GLuint vertex, GLuint fragment) {
    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::cerr << "[Shader] Program linking failed:\n" << infoLog << std::endl;
    }
}

void Shader::cleanup() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

} // namespace voxel
