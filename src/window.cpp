#include "window.h"

namespace wgfx {

Window::Window(std::string title, int width, int height, GraphicsBackend backend)
    : width(width), height(height), backend(backend) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwDefaultWindowHints();
    if (backend == GraphicsBackend::OpenGL) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (backend == GraphicsBackend::OpenGL) {
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            glfwDestroyWindow(window);
            window = nullptr;
            glfwTerminate();
            return;
        }
    }

    glfwGetFramebufferSize(window, &this->width, &this->height);
    if (backend == GraphicsBackend::OpenGL) {
        glViewport(0, 0, this->width, this->height);
        setVSync(true);
    }
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
VkExtent2D Window::getExtent() const {
    return {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
}

bool Window::isVulkanSupported() {
    if (!glfwInit()) {
        return false;
    }
    const bool supported = glfwVulkanSupported() == GLFW_TRUE;
    glfwTerminate();
    return supported;
}

GraphicsBackend Window::getBackend() const {
    return backend;
}

std::vector<const char*> Window::getRequiredVulkanInstanceExtensions() const {
    if (backend != GraphicsBackend::Vulkan) {
        return {};
    }
    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (extensions == nullptr || extensionCount == 0) {
        return {};
    }
    return std::vector<const char*>(extensions, extensions + extensionCount);
}

VkResult Window::createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) const {
    if (backend != GraphicsBackend::Vulkan || instance == VK_NULL_HANDLE || surface == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return glfwCreateWindowSurface(instance, window, nullptr, surface);
}

bool Window::isVSyncEnabled() const {
    return vsyncEnabled;
}

void Window::setVSync(bool enabled) {
    if (backend != GraphicsBackend::OpenGL) {
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(enabled ? 1 : 0);
    vsyncEnabled = enabled;
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto* owner = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (owner != nullptr) {
        owner->width = width;
        owner->height = height;
    }
    if (owner != nullptr && owner->backend == GraphicsBackend::OpenGL) {
        glViewport(0, 0, width, height);
    }
}

void Window::pollEvents() {
    if (backend == GraphicsBackend::OpenGL) {
        glfwSwapBuffers(window);
    }
    glfwPollEvents();
}

} // namespace wgfx
