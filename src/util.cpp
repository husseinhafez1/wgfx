#include "util.h"

#include <glad/glad.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace wgfx::util {
namespace {
const char* debugSourceName(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "window system";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "third party";
        case GL_DEBUG_SOURCE_APPLICATION: return "application";
        default: return "other";
    }
}

const char* debugTypeName(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: return "error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined behavior";
        case GL_DEBUG_TYPE_PORTABILITY: return "portability";
        case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
        case GL_DEBUG_TYPE_MARKER: return "marker";
        case GL_DEBUG_TYPE_PUSH_GROUP: return "push group";
        case GL_DEBUG_TYPE_POP_GROUP: return "pop group";
        default: return "other";
    }
}

const char* debugSeverityName(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: return "high";
        case GL_DEBUG_SEVERITY_MEDIUM: return "medium";
        case GL_DEBUG_SEVERITY_LOW: return "low";
        default: return "notification";
    }
}

void GLAPIENTRY openGLDebugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei,
    const GLchar* message,
    const void*
) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    std::cerr << "OpenGL " << debugTypeName(type)
              << " [" << debugSeverityName(severity) << "]"
              << " from " << debugSourceName(source)
              << " (" << id << "): " << message << '\n';
}
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open text file '" + path.string() + "'.");
    }
    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    if (size < 0) {
        throw std::runtime_error("Failed to determine text file size for '" + path.string() + "'.");
    }
    std::string contents(static_cast<std::size_t>(size), '\0');
    file.seekg(0);
    if (size > 0 && !file.read(contents.data(), size)) {
        throw std::runtime_error("Failed to read text file '" + path.string() + "'.");
    }
    return contents;
}

std::vector<char> readBinaryFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open binary file '" + path.string() + "'.");
    }
    const std::streamsize size = file.tellg();
    if (size < 0) {
        throw std::runtime_error("Failed to determine binary file size for '" + path.string() + "'.");
    }
    std::vector<char> contents(static_cast<std::size_t>(size));
    file.seekg(0);
    if (size > 0 && !file.read(contents.data(), size)) {
        throw std::runtime_error("Failed to read binary file '" + path.string() + "'.");
    }
    return contents;
}

void writeBinaryFile(const std::filesystem::path& path, std::span<const std::byte> data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Failed to open binary file for writing '" + path.string() + "'.");
    }
    if (!data.empty()) {
        file.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size_bytes())
        );
    }
    if (!file) {
        throw std::runtime_error("Failed to write binary file '" + path.string() + "'.");
    }
}

bool fileExists(const std::filesystem::path& path) noexcept {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

std::filesystem::path directoryOf(const std::filesystem::path& path) {
    const std::filesystem::path parent = path.parent_path();
    return parent.empty() ? std::filesystem::path(".") : parent;
}

std::int64_t currentTimeMilliseconds() noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void enableOpenGLDebugOutput() {
    if (!GLAD_GL_VERSION_4_3) {
        return;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(openGLDebugCallback, nullptr);
}

} // namespace wgfx::util
