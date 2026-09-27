// #include <renderer.h>
// #include <vk_renderer.h>
// #include <window.h>

// #include <algorithm>
// #include <cctype>
// #include <exception>
// #include <iostream>
// #include <stdexcept>
// #include <string>

#include <GLFW/glfw3.h>

#include <Instance/Instance.h>

#include <iostream>
#include <vector>

// namespace {
// enum class BackendSelection {
//     Automatic,
//     OpenGL,
//     Vulkan
// };

// BackendSelection parseBackend(int argc, char** argv) {
//     if (argc == 1) {
//         return BackendSelection::Automatic;
//     }
//     if (argc != 2) {
//         throw std::invalid_argument("Usage: wgfx [-V|-Vulkan|-O|-OpenGL]");
//     }

//     std::string argument = argv[1];
//     std::transform(argument.begin(), argument.end(), argument.begin(), [](unsigned char character) {
//         return static_cast<char>(std::tolower(character));
//     });
//     if (argument == "-v" || argument == "-vulkan") {
//         return BackendSelection::Vulkan;
//     }
//     if (argument == "-o" || argument == "-opengl") {
//         return BackendSelection::OpenGL;
//     }
//     throw std::invalid_argument("Unknown backend '" + argument + "'. Use -Vulkan or -OpenGL.");
// }

// int runOpenGL() {
//     wgfx::Renderer renderer;
//     renderer.init();
//     renderer.run();
//     return 0;
// }

// int runVulkan() {
//     wgfx::VkRenderer renderer;
//     renderer.init();
//     renderer.run();
//     return 0;
// }
// } // namespace

int main() {
    std::shared_ptr<Instance> instance = CreateInstance(ApiType::kVulkan);
    const std::vector<std::shared_ptr<Adapter>> adapters = instance->EnumerateAdapters();
    for (const std::shared_ptr<Adapter>& candidate : adapters) {
        std::cout << "FlyCube adapter: " << candidate->GetName() << '\n';
    }
    if (adapters.size() < 2) {
        std::cerr << "Expected at least 2 adapters.\n";
        return -1;
    }
    std::shared_ptr<Adapter> adapter = adapters[1];
    std::cout << "Selected adapter: " << adapter->GetName() << '\n';

    GLFWwindow* window;
    if (!glfwInit()) {
        return -1;
    }

    window = glfwCreateWindow(640, 480, "Hello World", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
