#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace voxel {

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Move-only
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLuint getID() const { return m_program; }

    // Uniform setters
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

    // Hot-reload from disk
    void reload();

private:
    GLuint m_program = 0;
    std::string m_vertexPath;
    std::string m_fragmentPath;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;

    GLint getUniformLocation(const std::string& name) const;
    GLuint compileShader(GLenum type, const std::string& source);
    std::string loadFile(const std::string& path);
    void linkProgram(GLuint vertex, GLuint fragment);
    void cleanup();
};

} // namespace voxel
