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
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include <Device/Device.h>
#include <Instance/Instance.h>
#include <Adapter/VKAdapter.h>

#include <cstdint>
#include <iostream>
#include <vector>

#include <array>
#include <glm/vec3.hpp>

namespace {

constexpr uint32_t kWidth = 640;
constexpr uint32_t kHeight = 480;
constexpr uint32_t kFrameCount = 3;

// Queried through the stable C API: vulkan.hpp's dynamic dispatcher asserts
// on the header version, which differs between our TUs and FlyCube's.
extern "C" void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                              VkPhysicalDeviceProperties* pProperties);

NativeSurface getNativeSurface(GLFWwindow* window) {
    return XlibSurface{
        .dpy = glfwGetX11Display(),
        .window = glfwGetX11Window(window),
    };
}

} // namespace

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
    std::shared_ptr<Adapter> adapter;
    for (const std::shared_ptr<Adapter>& candidate : adapters) {
        std::cout << "FlyCube adapter: " << candidate->GetName() << '\n';
        auto* vk_adapter = dynamic_cast<VKAdapter*>(candidate.get());
        if (vk_adapter == nullptr) {
            continue;
        }
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(
            static_cast<VkPhysicalDevice>(vk_adapter->GetPhysicalDevice()), &properties);
        if (properties.apiVersion >= VK_API_VERSION_1_4) {
            adapter = candidate;
            break;
        }
    }
    if (!adapter) {
        std::cerr << "No adapter with Vulkan 1.4 support found.\n";
        return -1;
    }
    std::cout << "Selected adapter: " << adapter->GetName() << '\n';

    GLFWwindow* window;
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(kWidth, kHeight, "Hello World", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    std::shared_ptr<Device> device = adapter->CreateDevice();
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    NativeSurface surface = getNativeSurface(window);
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(surface, kWidth, kHeight, kFrameCount, true);

    std::vector<uint32_t> index_data = {0,1,2};
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(MemoryType::kUpload, { .size = sizeof(index_data.front()) * index_data.size(), .usage = BindFlag::kIndexBuffer });
    index_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(MemoryType::kUpload, { .size = sizeof(vertex_data.front()) * vertex_data.size(), .usage = BindFlag::kVertexBuffer });
    vertex_buffer->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

    std::shared_ptr<Shader> vertex_shader = device->CompileShader({ "res/shaders/HLSL/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader({ "res/shaders/HLSL/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" });

    std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout({ .bind_keys = {} });
    GraphicsPipelineDesc pipeline_desc = {
        .shaders = { vertex_shader, pixel_shader },
        .layout = layout,
        .input = { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
        .color_formats = { swapchain->GetFormat() },
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists = {};
    std::array<uint64_t, kFrameCount> fence_values = {};
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        back_buffer_views[i] = device->CreateView(back_buffer, back_buffer_view_desc);

        std::shared_ptr<CommandList>& command_list = command_lists[i];
        command_list = device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline);
        command_list->SetViewport(0, 0, kWidth, kHeight, 0.0, 1.0);
        command_list->SetScissorRect(0, 0, kWidth, kHeight);
        command_list->IASetIndexBuffer(index_buffer, 0, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer, 0);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        RenderPassDesc render_pass_desc = {
            .render_area = { 0, 0, kWidth, kHeight },
            .colors = { { .view = back_buffer_views[i],
                          .load_op = RenderPassLoadOp::kClear,
                          .store_op = RenderPassStoreOp::kStore,
                          .clear_value = { 0.0, 0.0, 0.0, 1.0 } } },
        };
        command_list->BeginRenderPass(render_pass_desc);
        command_list->DrawIndexed(3, 1, 0, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        uint32_t frame = swapchain->NextImage(fence, ++fence_value);
        command_queue->Wait(fence, fence_value);
        fence->Wait(fence_values[frame]);
        command_queue->ExecuteCommandLists({ command_lists[frame] });
        command_queue->Signal(fence, fence_values[frame] = ++fence_value);
        swapchain->Present(fence, fence_values[frame]);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
