#include <pipeline.h>
#include <renderer.h>
#include <vulkan_device.h>
#include <window.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

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
    wgfx::VulkanDevice device(window);
    const wgfx::PipelineConfigInfo pipelineConfig =
        wgfx::Pipeline::defaultPipelineConfigInfo(1200, 800);
    wgfx::Pipeline pipeline(
        device,
        std::string(VULKAN_SHADER_DIR) + "basic.vert.spv",
        std::string(VULKAN_SHADER_DIR) + "basic.frag.spv",
        pipelineConfig
    );
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
