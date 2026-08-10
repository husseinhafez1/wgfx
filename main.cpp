#include <renderer.h>
#include <window.h>

#include <vulkan/vulkan_raii.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
enum class BackendSelection {
    Automatic,
    OpenGL,
    Vulkan
};

BackendSelection parseBackend(int argc, char** argv) {
    if (argc == 1) {
        return BackendSelection::Automatic;
    }
    if (argc != 2) {
        throw std::invalid_argument("Usage: wgfx [-V|-Vulkan|-O|-OpenGL]");
    }

    std::string argument = argv[1];
    std::transform(argument.begin(), argument.end(), argument.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (argument == "-v" || argument == "-vulkan") {
        return BackendSelection::Vulkan;
    }
    if (argument == "-o" || argument == "-opengl") {
        return BackendSelection::OpenGL;
    }
    throw std::invalid_argument("Unknown backend '" + argument + "'. Use -Vulkan or -OpenGL.");
}

int runOpenGL() {
    wgfx::Renderer renderer;
    renderer.init();
    renderer.run();
    return 0;
}

int runVulkan() {
    wgfx::Window window("wgfx - Vulkan", 1200, 800, wgfx::GraphicsBackend::Vulkan);
    if (!window.isOpen()) {
        throw std::runtime_error("Failed to create a Vulkan-compatible window.");
    }

    const std::vector<const char*> extensions = window.getRequiredVulkanInstanceExtensions();
    if (extensions.empty()) {
        throw std::runtime_error("GLFW did not provide the required Vulkan instance extensions.");
    }

    vk::raii::Context context;
    const uint32_t supportedVersion = context.enumerateInstanceVersion();
    const uint32_t requestedVersion = std::min(supportedVersion, vk::ApiVersion14);
    const vk::ApplicationInfo applicationInfo(
        "wgfx",
        VK_MAKE_VERSION(1, 0, 0),
        "wgfx",
        VK_MAKE_VERSION(1, 0, 0),
        requestedVersion
    );
    const vk::InstanceCreateInfo createInfo(
        {},
        &applicationInfo,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    );
    const vk::raii::Instance instance(context, createInfo);

    std::cout << "Using Vulkan "
              << VK_API_VERSION_MAJOR(requestedVersion) << '.'
              << VK_API_VERSION_MINOR(requestedVersion) << '\n';
    while (window.isOpen()) {
        window.pollEvents();
    }
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const BackendSelection selection = parseBackend(argc, argv);
        if (selection == BackendSelection::OpenGL) {
            return runOpenGL();
        }
        if (selection == BackendSelection::Vulkan) {
            if (!wgfx::Window::isVulkanSupported()) {
                throw std::runtime_error("Vulkan was requested but no Vulkan loader/driver is available.");
            }
            return runVulkan();
        }

        if (wgfx::Window::isVulkanSupported()) {
            try {
                return runVulkan();
            } catch (const std::exception& exception) {
                std::cerr << "Vulkan initialization failed: " << exception.what()
                          << "\nFalling back to OpenGL.\n";
            }
        } else {
            std::cout << "Vulkan is unavailable; falling back to OpenGL.\n";
        }
        return runOpenGL();
    } catch (const std::exception& exception) {
        std::cerr << "Application error: " << exception.what() << '\n';
        return -1;
    }
}
