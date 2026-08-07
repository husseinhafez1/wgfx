#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

class Shader {
public:
    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    void use() const;
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, const glm::vec3& value);
    void setUniform(const std::string& name, const glm::mat4& value);
    void recompile();

private:
    unsigned int ID;
    std::string vertexPath;
    std::string fragmentPath;
    std::unordered_map<std::string, int> uniformLocations;
    int getUniformLocation(const std::string& name);
    std::string readShaderSource(const std::string& filePath) const;
    void checkCompileErrors(unsigned int shader, const std::string& type) const;
};
