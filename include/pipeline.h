#pragma once

#include "vulkan_device.h"

#include <cstdint>
#include <string>
#include <vector>

namespace wgfx {

struct PipelineConfigInfo {
    VkViewport viewport{};
    VkRect2D scissor{};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
};

class Pipeline {
public:
    Pipeline(
        VulkanDevice& device,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath,
        const PipelineConfigInfo& configInfo
    );
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

private:
    static std::vector<char> readFile(const std::string& filePath);
    void createGraphicsPipeline(
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath,
        const PipelineConfigInfo& configInfo
    );
    VkShaderModule createShaderModule(const std::vector<char>& code) const;

    VulkanDevice& device;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
};

} // namespace wgfx
