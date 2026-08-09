#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>

namespace wgfx {

class Window {
public:
    Window(std::string title = "window", int width = 800, int height = 600);
    ~Window();

    [[nodiscard]] GLFWwindow* get() const;
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;
    [[nodiscard]] bool isVSyncEnabled() const;
    void setVSync(bool enabled);
    void getCursorPos(int& x, int& y);
    void pollEvents();

private:
    GLFWwindow* window = nullptr;
    int width;
    int height;
    bool vsyncEnabled = false;
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};

} // namespace wgfx
