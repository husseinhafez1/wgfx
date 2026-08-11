#pragma once

#include "vulkan_device.h"

#include <cstddef>
#include <vector>

namespace wgfx {

class SwapChain {
public:
    static constexpr size_t MaxFramesInFlight = 2;

    SwapChain(VulkanDevice& device, VkExtent2D windowExtent);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;
    SwapChain(SwapChain&&) = delete;
    SwapChain& operator=(SwapChain&&) = delete;

    [[nodiscard]] VkFramebuffer getFrameBuffer(size_t index) const;
    [[nodiscard]] VkRenderPass getRenderPass() const;
    [[nodiscard]] VkImageView getImageView(size_t index) const;
    [[nodiscard]] size_t imageCount() const;
    [[nodiscard]] VkFormat getImageFormat() const;
    [[nodiscard]] VkExtent2D getExtent() const;
    [[nodiscard]] uint32_t width() const;
    [[nodiscard]] uint32_t height() const;
    [[nodiscard]] float extentAspectRatio() const;
    [[nodiscard]] VkFormat findDepthFormat() const;

    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

private:
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();
    void cleanup();

    [[nodiscard]] VkSurfaceFormatKHR chooseSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    ) const;
    [[nodiscard]] VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    ) const;
    [[nodiscard]] VkExtent2D chooseExtent(
        const VkSurfaceCapabilitiesKHR& capabilities
    ) const;

    VulkanDevice& device;
    VkExtent2D windowExtent{};
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemories;
    std::vector<VkImageView> depthImageViews;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

} // namespace wgfx
