// #include <renderer.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <exception>
#include <iostream>

GLFWwindow* initWindow(const char* title, uint32_t& width, uint32_t height) {
    glfwSetErrorCallback([](int error, const char* description){
        printf("GLFW Error (%i): %s\n", error, description);
    });
    if (!glfwInit()) return nullptr;
    const bool wantsWholeArea = !width || !height;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, wantsWholeArea ? GLFW_FALSE : GLFW_TRUE);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int x(0), y(0);
    int w = mode->width;
    int h = mode->height;
    if (wantsWholeArea) {
        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
    } else {
        w = width;
        h = height;
    }
    GLFWwindow* window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }
    if (wantsWholeArea) glfwSetWindowPos(window, w, h);
    glfwGetWindowSize(window, &w, &h);
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    });
    return window;
}

int main() {
    uint32_t width = 1280;
    uint32_t height = 800;
    GLFWwindow* window = initWindow("wgfx", width, height);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    // try {
    //     // wgfx::Renderer renderer;
    //     // renderer.init();
    //     // renderer.run();
    // } catch (const std::exception& exception) {
    //     std::cerr << "Application error: " << exception.what() << '\n';
    //     return -1;
    // }
    return 0;
}
