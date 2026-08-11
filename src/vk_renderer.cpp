#include "vk_renderer.h"

#include "pipeline.h"
#include "swap_chain.h"
#include "vulkan_device.h"
#include "window.h"

#include <stdexcept>
#include <string>
#include <array>

namespace wgfx {

VkRenderer::VkRenderer() = default;

VkRenderer::~VkRenderer() {
    if (device != nullptr) {
        vkDeviceWaitIdle(device->getDevice());
    }
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
        drawFrame();
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

    for (int i = 0; i < commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapChain->getRenderPass();
        renderPassInfo.framebuffer = swapChain->getFrameBuffer(i);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain->getExtent();

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        pipeline->bind(commandBuffers[i]);
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }
}


void VkRenderer::drawFrame() {
    uint32_t imageIndex;
    auto result = swapChain->acquireNextImage(&imageIndex);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    result = swapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swapchain image");
    }
}

} // namespace wgfx
