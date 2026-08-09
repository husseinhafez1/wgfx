#include "shader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
std::string readShaderFile(
    const std::filesystem::path& path,
    std::vector<std::filesystem::path>& includeStack
) {
    const std::filesystem::path normalizedPath = path.lexically_normal();
    if (std::find(includeStack.begin(), includeStack.end(), normalizedPath) != includeStack.end()) {
        throw std::runtime_error("Shader include cycle at '" + normalizedPath.string() + "'.");
    }

    std::ifstream file(normalizedPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file '" + normalizedPath.string() + "'.");
    }

    includeStack.push_back(normalizedPath);
    std::stringstream source;
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t directive = line.find("#include");
        if (directive != std::string::npos && line.find_first_not_of(" \t") == directive) {
            const std::size_t openingQuote = line.find('"', directive);
            if (openingQuote == std::string::npos) {
                throw std::runtime_error("Malformed shader include in '" + normalizedPath.string() + "'.");
            }
            const std::size_t closingQuote = line.find('"', openingQuote + 1);
            if (closingQuote == std::string::npos) {
                throw std::runtime_error("Malformed shader include in '" + normalizedPath.string() + "'.");
            }
            const std::filesystem::path includedPath = line.substr(
                openingQuote + 1,
                closingQuote - openingQuote - 1
            );
            source << readShaderFile(normalizedPath.parent_path() / includedPath, includeStack);
        } else {
            source << line << '\n';
        }
    }
    includeStack.pop_back();
    return source.str();
}
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    this->vertexPath = vertexPath;
    this->fragmentPath = fragmentPath;

    std::string vertexCode = readShaderSource(vertexPath);
    std::string fragmentCode = readShaderSource(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // Shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setUniform(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string& name, const glm::vec3& value) {
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setUniform(const std::string& name, const glm::vec4& value) {
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setUniform(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

int Shader::getUniformLocation(const std::string& name) {
    const auto existing = uniformLocations.find(name);
    if (existing != uniformLocations.end()) {
        return existing->second;
    }

    const int location = glGetUniformLocation(ID, name.c_str());
    uniformLocations.emplace(name, location);
    return location;
}

void Shader::recompile() {
    std::string vertexCode = readShaderSource(vertexPath);
    std::string fragmentCode = readShaderSource(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // Shader Program
    unsigned int newID = glCreateProgram();
    glAttachShader(newID, vertex);
    glAttachShader(newID, fragment);
    glLinkProgram(newID);
    checkCompileErrors(newID, "PROGRAM");

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // Delete the old program and update the ID
    glDeleteProgram(ID);
    ID = newID;

    // Clear uniform locations cache
    uniformLocations.clear();
}

std::string Shader::readShaderSource(const std::string& filePath) const {
    std::vector<std::filesystem::path> includeStack;
    return readShaderFile(std::filesystem::path(SHADER_DIR) / filePath, includeStack);
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type) const {
    int success;
    char infoLog[1024];

    if (type == "PROGRAM") {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << '\n';
        }
        return;
    }

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "ERROR::SHADER::" << type << "::COMPILATION_FAILED\n"
                  << infoLog << '\n';
    }
}
