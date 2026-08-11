#include "pipeline.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cassert>

namespace wgfx {

Pipeline::Pipeline(
    VulkanDevice& device,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath,
    const PipelineConfigInfo& configInfo
) : device(device) {
    createGraphicsPipeline(vertexShaderPath, fragmentShaderPath, configInfo);
}

Pipeline::~Pipeline() {
    const VkDevice logicalDevice = device.getDevice();
    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    }
    if (vertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
    }
    if (fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
    }
}

std::vector<char> Pipeline::readFile(const std::string& filePath) {
    const std::filesystem::path resolvedPath = std::filesystem::path(SHADER_DIR) / filePath;
    std::ifstream file(resolvedPath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + resolvedPath.string());
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0 || fileSize % 4 != 0) {
        throw std::runtime_error("Invalid SPIR-V shader file: " + resolvedPath.string());
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.seekg(0);
    if (!file.read(buffer.data(), fileSize)) {
        throw std::runtime_error("Failed to read shader file: " + resolvedPath.string());
    }
    return buffer;
}

void Pipeline::createGraphicsPipeline(
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath,
    const PipelineConfigInfo& configInfo
) {
    assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "can't create graphics pipeline with no pipeline layout provided in configInfo");
    assert(configInfo.renderPass != VK_NULL_HANDLE && "can't create graphics pipeline with no pipeline layout provided in configInfo");

    const std::vector<char> vertexCode = readFile(vertexShaderPath);
    const std::vector<char> fragmentCode = readFile(fragmentShaderPath);

    vertexShaderModule = createShaderModule(vertexCode);
    fragmentShaderModule = createShaderModule(fragmentCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pSpecializationInfo = nullptr;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &configInfo.viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &configInfo.scissor;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = configInfo.pipelineLayout;
    pipelineInfo.renderPass = configInfo.renderPass;
    pipelineInfo.subpass = configInfo.subpass;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create a graphics pipeline");
    }
}

VkShaderModule Pipeline::createShaderModule(const std::vector<char>& code) const {
    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan shader module.");
    }
    return shaderModule;
}

void Pipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

PipelineConfigInfo Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
    PipelineConfigInfo configInfo{};

    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    configInfo.viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    configInfo.scissor = {
        .offset = {0, 0},
        .extent = {width, height}
    };

    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;

    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    configInfo.colorBlendAttachment.blendEnable = VK_FALSE;

    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.attachmentCount = 1;
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;

    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;

    return configInfo;
}

} // namespace wgfx
