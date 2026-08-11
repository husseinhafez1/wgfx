#include "swap_chain.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace wgfx {

SwapChain::SwapChain(VulkanDevice& device, VkExtent2D windowExtent)
    : device(device), windowExtent(windowExtent) {
    try {
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDepthResources();
        createFramebuffers();
        createSyncObjects();
    } catch (...) {
        cleanup();
        throw;
    }
}

SwapChain::~SwapChain() {
    vkDeviceWaitIdle(device.getDevice());
    cleanup();
}

VkFramebuffer SwapChain::getFrameBuffer(size_t index) const {
    return framebuffers.at(index);
}

VkRenderPass SwapChain::getRenderPass() const { return renderPass; }

VkImageView SwapChain::getImageView(size_t index) const {
    return imageViews.at(index);
}

size_t SwapChain::imageCount() const { return images.size(); }
VkFormat SwapChain::getImageFormat() const { return imageFormat; }
VkExtent2D SwapChain::getExtent() const { return extent; }
uint32_t SwapChain::width() const { return extent.width; }
uint32_t SwapChain::height() const { return extent.height; }

float SwapChain::extentAspectRatio() const {
    return static_cast<float>(extent.width) / static_cast<float>(extent.height);
}

VkFormat SwapChain::findDepthFormat() const {
    return device.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkResult SwapChain::acquireNextImage(uint32_t* imageIndex) {
    vkWaitForFences(
        device.getDevice(),
        1,
        &inFlightFences[currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max()
    );
    return vkAcquireNextImageKHR(
        device.getDevice(),
        swapChain,
        std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        imageIndex
    );
}

VkResult SwapChain::submitCommandBuffers(
    const VkCommandBuffer* buffers,
    uint32_t* imageIndex
) {
    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(
            device.getDevice(),
            1,
            &imagesInFlight[*imageIndex],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max()
        );
    }
    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = buffers,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    vkResetFences(device.getDevice(), 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(
            device.getGraphicsQueue(),
            1,
            &submitInfo,
            inFlightFences[currentFrame]
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit Vulkan command buffer.");
    }

    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = imageIndex,
        .pResults = nullptr
    };
    const VkResult result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
    currentFrame = (currentFrame + 1) % MaxFramesInFlight;
    return result;
}

void SwapChain::createSwapChain() {
    const SwapChainSupportDetails support = device.getSwapChainSupport();
    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
    const VkExtent2D selectedExtent = chooseExtent(support.capabilities);

    uint32_t requestedImageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0) {
        requestedImageCount = std::min(requestedImageCount, support.capabilities.maxImageCount);
    }

    const QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
    const uint32_t queueFamilies[] = {indices.graphicsFamily, indices.presentFamily};
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = device.getSurface(),
        .minImageCount = requestedImageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = selectedExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = support.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilies;
    }

    if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swap chain.");
    }

    uint32_t actualImageCount = 0;
    vkGetSwapchainImagesKHR(device.getDevice(), swapChain, &actualImageCount, nullptr);
    images.resize(actualImageCount);
    vkGetSwapchainImagesKHR(device.getDevice(), swapChain, &actualImageCount, images.data());
    imageFormat = surfaceFormat.format;
    extent = selectedExtent;
}

void SwapChain::createImageViews() {
    imageViews.resize(images.size(), VK_NULL_HANDLE);
    for (size_t index = 0; index < images.size(); ++index) {
        const VkImageViewCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = images[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageFormat,
            .components = {},
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        if (vkCreateImageView(
                device.getDevice(),
                &createInfo,
                nullptr,
                &imageViews[index]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap-chain image view.");
        }
    }
}

void SwapChain::createRenderPass() {
    const VkAttachmentDescription colorAttachment{
        .flags = 0,
        .format = imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    const VkAttachmentDescription depthAttachment{
        .flags = 0,
        .format = findDepthFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference colorReference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference depthReference{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    const VkSubpassDescription subpass{
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorReference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = &depthReference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };
    const VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };
    const std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment,
        depthAttachment
    };
    const VkRenderPassCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    if (vkCreateRenderPass(device.getDevice(), &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan render pass.");
    }
}

void SwapChain::createDepthResources() {
    const VkFormat depthFormat = findDepthFormat();
    depthImages.resize(imageCount(), VK_NULL_HANDLE);
    depthImageMemories.resize(imageCount(), VK_NULL_HANDLE);
    depthImageViews.resize(imageCount(), VK_NULL_HANDLE);

    for (size_t index = 0; index < imageCount(); ++index) {
        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = depthFormat,
            .extent = {extent.width, extent.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };
        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImages[index],
            depthImageMemories[index]
        );

        const VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = depthImages[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depthFormat,
            .components = {},
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        if (vkCreateImageView(
                device.getDevice(),
                &viewInfo,
                nullptr,
                &depthImageViews[index]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create depth image view.");
        }
    }
}

void SwapChain::createFramebuffers() {
    framebuffers.resize(imageCount(), VK_NULL_HANDLE);
    for (size_t index = 0; index < imageCount(); ++index) {
        const std::array<VkImageView, 2> attachments = {
            imageViews[index],
            depthImageViews[index]
        };
        const VkFramebufferCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };
        if (vkCreateFramebuffer(
                device.getDevice(),
                &createInfo,
                nullptr,
                &framebuffers[index]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap-chain framebuffer.");
        }
    }
}

void SwapChain::createSyncObjects() {
    imageAvailableSemaphores.resize(MaxFramesInFlight, VK_NULL_HANDLE);
    renderFinishedSemaphores.resize(MaxFramesInFlight, VK_NULL_HANDLE);
    inFlightFences.resize(MaxFramesInFlight, VK_NULL_HANDLE);
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

    const VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    const VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for (size_t index = 0; index < MaxFramesInFlight; ++index) {
        if (vkCreateSemaphore(
                device.getDevice(),
                &semaphoreInfo,
                nullptr,
                &imageAvailableSemaphores[index]
            ) != VK_SUCCESS ||
            vkCreateSemaphore(
                device.getDevice(),
                &semaphoreInfo,
                nullptr,
                &renderFinishedSemaphores[index]
            ) != VK_SUCCESS ||
            vkCreateFence(
                device.getDevice(),
                &fenceInfo,
                nullptr,
                &inFlightFences[index]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create frame synchronization objects.");
        }
    }
}

VkSurfaceFormatKHR SwapChain::chooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats
) const {
    for (const VkSurfaceFormatKHR& format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return availableFormats.front();
}

VkPresentModeKHR SwapChain::choosePresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes
) const {
    for (const VkPresentModeKHR mode : availablePresentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            std::cout << "Vulkan present mode: Mailbox\n";
            return mode;
        }
    }
    std::cout << "Vulkan present mode: V-Sync\n";
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    return {
        std::clamp(
            windowExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        ),
        std::clamp(
            windowExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        )
    };
}

void SwapChain::cleanup() {
    const VkDevice logicalDevice = device.getDevice();

    for (VkFence fence : inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(logicalDevice, fence, nullptr);
        }
    }
    for (VkSemaphore semaphore : renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(logicalDevice, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : imageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(logicalDevice, semaphore, nullptr);
        }
    }
    for (VkFramebuffer framebuffer : framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }
    }
    for (VkImageView view : depthImageViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(logicalDevice, view, nullptr);
        }
    }
    for (VkImage image : depthImages) {
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(logicalDevice, image, nullptr);
        }
    }
    for (VkDeviceMemory memory : depthImageMemories) {
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice, memory, nullptr);
        }
    }
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
    for (VkImageView view : imageViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(logicalDevice, view, nullptr);
        }
    }
    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
}

} // namespace wgfx
