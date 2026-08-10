#pragma once

#include "window.h"

#include <cstdint>
#include <vector>

namespace wgfx {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;

    [[nodiscard]] bool isComplete() const {
        return graphicsFamilyHasValue && presentFamilyHasValue;
    }
};

class VulkanDevice {
public:
#ifdef NDEBUG
    static constexpr bool EnableValidationLayers = false;
#else
    static constexpr bool EnableValidationLayers = true;
#endif

    explicit VulkanDevice(Window& window);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    [[nodiscard]] VkCommandPool getCommandPool() const;
    [[nodiscard]] VkDevice getDevice() const;
    [[nodiscard]] VkSurfaceKHR getSurface() const;
    [[nodiscard]] VkQueue getGraphicsQueue() const;
    [[nodiscard]] VkQueue getPresentQueue() const;
    [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const;
    [[nodiscard]] QueueFamilyIndices findPhysicalQueueFamilies() const;

    [[nodiscard]] uint32_t findMemoryType(
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    ) const;
    [[nodiscard]] VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    ) const;

    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    ) const;
    [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void copyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size) const;
    void copyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height,
        uint32_t layerCount
    ) const;
    void createImageWithInfo(
        const VkImageCreateInfo& imageInfo,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory
    ) const;

    VkPhysicalDeviceProperties properties{};

private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    [[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device) const;
    [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
    [[nodiscard]] bool checkValidationLayerSupport() const;
    [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo
    ) const;
    void verifyGlfwRequiredInstanceExtensions() const;
    [[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(
        VkPhysicalDevice device
    ) const;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    Window& window;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};

} // namespace wgfx
