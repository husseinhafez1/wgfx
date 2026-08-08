#include "window.h"

Window::Window(std::string title, int width, int height) : width(width), height(height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        window = nullptr;
        glfwTerminate();
        return;
    }

    glfwGetFramebufferSize(window, &this->width, &this->height);
    glViewport(0, 0, this->width, this->height);
}

Window::~Window() {
    if (window != nullptr) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

GLFWwindow* Window::get() const {
    return window;
}
bool Window::isOpen() const {return window != nullptr && !glfwWindowShouldClose(window);}
int Window::getWidth() const {return width;}
int Window::getHeight() const {return height;}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto* owner = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (owner != nullptr) {
        owner->width = width;
        owner->height = height;
    }
    glViewport(0, 0, width, height);
}

void Window::pollEvents() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}
