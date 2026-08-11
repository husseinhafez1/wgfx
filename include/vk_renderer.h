#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace wgfx {

class Pipeline;
class SwapChain;
class VulkanDevice;
class Window;

class VkRenderer {
public:
    VkRenderer();
    ~VkRenderer();

    VkRenderer(const VkRenderer&) = delete;
    VkRenderer& operator=(const VkRenderer&) = delete;
    VkRenderer(VkRenderer&&) = delete;
    VkRenderer& operator=(VkRenderer&&) = delete;

    void init();
    void run();

private:
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    void drawFrame();

    std::unique_ptr<Window> window;
    std::unique_ptr<VulkanDevice> device;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    bool initialized = false;
};

} // namespace wgfx
