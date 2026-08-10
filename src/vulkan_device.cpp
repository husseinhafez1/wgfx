#include "vulkan_device.h"

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace wgfx {
namespace {
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*
) {
    std::cerr << "validation layer: " << callbackData->pMessage << '\n';
    return VK_FALSE;
}

VkResult createDebugUtilsMessenger(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    VkDebugUtilsMessengerEXT* messenger
) {
    const auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
    );
    return function != nullptr
        ? function(instance, createInfo, nullptr, messenger)
        : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger
) {
    const auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (function != nullptr) {
        function(instance, messenger, nullptr);
    }
}
}

VulkanDevice::VulkanDevice(Window& window) : window(window) {
    if (window.getBackend() != GraphicsBackend::Vulkan) {
        throw std::invalid_argument("VulkanDevice requires a Vulkan window.");
    }
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}

VulkanDevice::~VulkanDevice() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
        vkDestroyDevice(device, nullptr);
    }
    if (EnableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
        destroyDebugUtilsMessenger(instance, debugMessenger);
    }
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}

VkCommandPool VulkanDevice::getCommandPool() const { return commandPool; }
VkDevice VulkanDevice::getDevice() const { return device; }
VkSurfaceKHR VulkanDevice::getSurface() const { return surface; }
VkQueue VulkanDevice::getGraphicsQueue() const { return graphicsQueue; }
VkQueue VulkanDevice::getPresentQueue() const { return presentQueue; }

SwapChainSupportDetails VulkanDevice::getSwapChainSupport() const {
    return querySwapChainSupport(physicalDevice);
}

QueueFamilyIndices VulkanDevice::findPhysicalQueueFamilies() const {
    return findQueueFamilies(physicalDevice);
}

void VulkanDevice::createInstance() {
    if (EnableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but unavailable.");
    }

    const VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "wgfx",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "wgfx",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    const std::vector<const char*> extensions = getRequiredExtensions();
    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
    verifyGlfwRequiredInstanceExtensions();
}

void VulkanDevice::setupDebugMessenger() {
    if (!EnableValidationLayers) {
        return;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    if (createDebugUtilsMessenger(instance, &createInfo, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan debug messenger.");
    }
}

void VulkanDevice::createSurface() {
    if (window.createVulkanSurface(instance, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan window surface.");
    }
}

void VulkanDevice::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-capable GPU was found.");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (VkPhysicalDevice candidate : devices) {
        if (isDeviceSuitable(candidate)) {
            physicalDevice = candidate;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable Vulkan GPU was found.");
    }
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    std::cout << "Vulkan physical device: " << properties.deviceName << '\n';
}

void VulkanDevice::createLogicalDevice() {
    const QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    const std::set<uint32_t> uniqueFamilies = {
        indices.graphicsFamily,
        indices.presentFamily
    };
    const float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t family : uniqueFamilies) {
        queueCreateInfos.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &priority
        });
    }

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &features
    };
    if (EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan logical device.");
    }
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

void VulkanDevice::createCommandPool() {
    const QueueFamilyIndices indices = findPhysicalQueueFamilies();
    const VkCommandPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
               | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphicsFamily
    };
    if (vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan command pool.");
    }
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice candidate) const {
    const QueueFamilyIndices indices = findQueueFamilies(candidate);
    if (!indices.isComplete() || !checkDeviceExtensionSupport(candidate)) {
        return false;
    }
    const SwapChainSupportDetails swapChain = querySwapChainSupport(candidate);
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(candidate, &features);
    return !swapChain.formats.empty() && !swapChain.presentModes.empty()
        && features.samplerAnisotropy == VK_TRUE;
}

std::vector<const char*> VulkanDevice::getRequiredExtensions() const {
    std::vector<const char*> extensions = window.getRequiredVulkanInstanceExtensions();
    if (extensions.empty()) {
        throw std::runtime_error("GLFW returned no Vulkan instance extensions.");
    }
    if (EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

bool VulkanDevice::checkValidationLayerSupport() const {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (const char* required : validationLayers) {
        bool found = false;
        for (const VkLayerProperties& available : layers) {
            if (std::strcmp(required, available.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice candidate) const {
    QueueFamilyIndices indices;
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
    for (uint32_t index = 0; index < familyCount; ++index) {
        if (families[index].queueCount > 0
            && (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            indices.graphicsFamily = index;
            indices.graphicsFamilyHasValue = true;
        }
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, index, surface, &presentSupport);
        if (families[index].queueCount > 0 && presentSupport == VK_TRUE) {
            indices.presentFamily = index;
            indices.presentFamilyHasValue = true;
        }
        if (indices.isComplete()) {
            break;
        }
    }
    return indices;
}

void VulkanDevice::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo
) const {
    createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr
    };
}

void VulkanDevice::verifyGlfwRequiredInstanceExtensions() const {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(
        nullptr,
        &extensionCount,
        availableExtensions.data()
    );
    std::unordered_set<std::string> available;
    for (const VkExtensionProperties& extension : availableExtensions) {
        available.emplace(extension.extensionName);
    }
    for (const char* required : getRequiredExtensions()) {
        if (available.find(required) == available.end()) {
            throw std::runtime_error(
                "Missing required Vulkan instance extension: " + std::string(required)
            );
        }
    }
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice candidate) const {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, available.data());
    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
    for (const VkExtensionProperties& extension : available) {
        required.erase(extension.extensionName);
    }
    return required.empty();
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(VkPhysicalDevice candidate) const {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, surface, &details.capabilities);
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &formatCount, nullptr);
    details.formats.resize(formatCount);
    if (formatCount > 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            candidate,
            surface,
            &formatCount,
            details.formats.data()
        );
    }
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &modeCount, nullptr);
    details.presentModes.resize(modeCount);
    if (modeCount > 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            candidate,
            surface,
            &modeCount,
            details.presentModes.data()
        );
    }
    return details;
}

VkFormat VulkanDevice::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features
) const {
    for (VkFormat format : candidates) {
        VkFormatProperties formatProperties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
        if (tiling == VK_IMAGE_TILING_LINEAR
            && (formatProperties.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL
            && (formatProperties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("Failed to find a supported Vulkan format.");
}

uint32_t VulkanDevice::findMemoryType(
    uint32_t typeFilter,
    VkMemoryPropertyFlags requiredProperties
) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        if ((typeFilter & (1U << index)) != 0
            && (memoryProperties.memoryTypes[index].propertyFlags & requiredProperties)
                == requiredProperties) {
            return index;
        }
    }
    throw std::runtime_error("Failed to find a suitable Vulkan memory type.");
}

void VulkanDevice::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
) const {
    const VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan buffer.");
    }
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const VkMemoryAllocateInfo allocationInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, memoryProperties)
    };
    if (vkAllocateMemory(device, &allocationInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to allocate Vulkan buffer memory.");
    }
    if (vkBindBufferMemory(device, buffer, bufferMemory, 0) != VK_SUCCESS) {
        vkFreeMemory(device, bufferMemory, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
        bufferMemory = VK_NULL_HANDLE;
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to bind Vulkan buffer memory.");
    }
}

VkCommandBuffer VulkanDevice::beginSingleTimeCommands() const {
    const VkCommandBufferAllocateInfo allocationInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocationInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan command buffer.");
    }
    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        throw std::runtime_error("Failed to begin Vulkan command buffer.");
    }
    return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end Vulkan command buffer.");
    }
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS
        || vkQueueWaitIdle(graphicsQueue) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit Vulkan command buffer.");
    }
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanDevice::copyBuffer(
    VkBuffer source,
    VkBuffer destination,
    VkDeviceSize size
) const {
    const VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    const VkBufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, source, destination, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer);
}

void VulkanDevice::copyBufferToImage(
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height,
    uint32_t layerCount
) const {
    const VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    const VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = layerCount
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}
    };
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    endSingleTimeCommands(commandBuffer);
}

void VulkanDevice::createImageWithInfo(
    const VkImageCreateInfo& imageInfo,
    VkMemoryPropertyFlags memoryProperties,
    VkImage& image,
    VkDeviceMemory& imageMemory
) const {
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image.");
    }
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, image, &requirements);
    const VkMemoryAllocateInfo allocationInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, memoryProperties)
    };
    if (vkAllocateMemory(device, &allocationInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to allocate Vulkan image memory.");
    }
    if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS) {
        vkFreeMemory(device, imageMemory, nullptr);
        vkDestroyImage(device, image, nullptr);
        imageMemory = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to bind Vulkan image memory.");
    }
}

} // namespace wgfx
