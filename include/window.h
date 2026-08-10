#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <vector>

namespace wgfx {

enum class GraphicsBackend {
    OpenGL,
    Vulkan
};

class Window {
public:
    Window(
        std::string title = "window",
        int width = 800,
        int height = 600,
        GraphicsBackend backend = GraphicsBackend::OpenGL
    );
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] static bool isVulkanSupported();
    [[nodiscard]] GLFWwindow* get() const;
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;
    [[nodiscard]] GraphicsBackend getBackend() const;
    [[nodiscard]] std::vector<const char*> getRequiredVulkanInstanceExtensions() const;
    [[nodiscard]] bool isVSyncEnabled() const;
    void setVSync(bool enabled);
    void getCursorPos(int& x, int& y);
    void pollEvents();

private:
    GLFWwindow* window = nullptr;
    int width;
    int height;
    GraphicsBackend backend;
    bool vsyncEnabled = false;
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};

} // namespace wgfx
