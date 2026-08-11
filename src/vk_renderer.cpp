#include "vk_renderer.h"

#include "pipeline.h"
#include "swap_chain.h"
#include "vulkan_device.h"
#include "window.h"

#include <stdexcept>
#include <string>

namespace wgfx {

VkRenderer::VkRenderer() = default;

VkRenderer::~VkRenderer() {
    pipeline.reset();
    if (device != nullptr && pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device->getDevice(), pipelineLayout, nullptr);
    }
}

void VkRenderer::init() {
    if (initialized) {
        return;
    }

    window = std::make_unique<Window>(
        "wgfx - Vulkan",
        1200,
        800,
        GraphicsBackend::Vulkan
    );
    if (!window->isOpen()) {
        throw std::runtime_error("Failed to create a Vulkan-compatible window.");
    }

    device = std::make_unique<VulkanDevice>(*window);
    swapChain = std::make_unique<SwapChain>(*device, window->getExtent());
    createPipelineLayout();
    createPipeline();
    createCommandBuffers();
    initialized = true;
}

void VkRenderer::run() {
    if (!initialized) {
        throw std::runtime_error("VkRenderer must be initialized before running.");
    }
    while (window->isOpen()) {
        window->pollEvents();
    }
}

void VkRenderer::createPipelineLayout() {
    const VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    if (vkCreatePipelineLayout(
            device->getDevice(),
            &createInfo,
            nullptr,
            &pipelineLayout
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan pipeline layout.");
    }
}

void VkRenderer::createPipeline() {
    auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(swapChain->width(), swapChain->height());
    pipelineConfig.renderPass = swapChain->getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipeline = std::make_unique<Pipeline>(
        *device,
        "vulkan/basic.vert.spv",
        "vulkan/basic.frag.spv",
        pipelineConfig
    );
}

void VkRenderer::createCommandBuffers() {
    commandBuffers.resize(swapChain->imageCount());
    const VkCommandBufferAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = device->getCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())
    };
    if (vkAllocateCommandBuffers(
            device->getDevice(),
            &allocateInfo,
            commandBuffers.data()
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan command buffers.");
    }
}


void VkRenderer::drawFrame() {}

} // namespace wgfx
