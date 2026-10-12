#include "Aroxorn/RayTracingPipeline.hpp"
#include <fstream>
#include <stdexcept>

namespace Aroxorn {

    RayTracingPipeline::RayTracingPipeline(Device& device, const std::string& rgenShader, const std::string& rmissShader, const std::string& rchitShader)
        : m_Device(device) {

        auto rgenCode = ReadFile(rgenShader);
        auto rmissCode = ReadFile(rmissShader);
        auto rchitCode = ReadFile(rchitShader);

        VkShaderModule rgenModule = CreateShaderModule(rgenCode);
        VkShaderModule rmissModule = CreateShaderModule(rmissCode);
        VkShaderModule rchitModule = CreateShaderModule(rchitCode);

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        // Ray Generation
        VkPipelineShaderStageCreateInfo rgenStage{};
        rgenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rgenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        rgenStage.module = rgenModule;
        rgenStage.pName = "main";
        shaderStages.push_back(rgenStage);

        // Ray Miss
        VkPipelineShaderStageCreateInfo rmissStage{};
        rmissStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rmissStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        rmissStage.module = rmissModule;
        rmissStage.pName = "main";
        shaderStages.push_back(rmissStage);

        // Ray Closest Hit
        VkPipelineShaderStageCreateInfo rchitStage{};
        rchitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rchitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        rchitStage.module = rchitModule;
        rchitStage.pName = "main";
        shaderStages.push_back(rchitStage);

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

        // Raygen group
        VkRayTracingShaderGroupCreateInfoKHR group0{};
        group0.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group0.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group0.generalShader = 0;
        group0.closestHitShader = VK_SHADER_UNUSED_KHR;
        group0.anyHitShader = VK_SHADER_UNUSED_KHR;
        group0.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(group0);

        // Miss group
        VkRayTracingShaderGroupCreateInfoKHR group1{};
        group1.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group1.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group1.generalShader = 1;
        group1.closestHitShader = VK_SHADER_UNUSED_KHR;
        group1.anyHitShader = VK_SHADER_UNUSED_KHR;
        group1.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(group1);

        // Hit group
        VkRayTracingShaderGroupCreateInfoKHR group2{};
        group2.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group2.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        group2.generalShader = VK_SHADER_UNUSED_KHR;
        group2.closestHitShader = 2;
        group2.anyHitShader = VK_SHADER_UNUSED_KHR;
        group2.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(group2);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(m_Device.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ray tracing pipeline layout!");
        }

        VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
        pipelineInfo.pGroups = shaderGroups.data();
        pipelineInfo.maxPipelineRayRecursionDepth = 1;
        pipelineInfo.layout = m_PipelineLayout;

        auto vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkCreateRayTracingPipelinesKHR");

        if (!vkCreateRayTracingPipelinesKHR) {
            throw std::runtime_error("Failed to load vkCreateRayTracingPipelinesKHR extension!");
        }

        if (vkCreateRayTracingPipelinesKHR(m_Device.GetVkDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ray tracing pipeline!");
        }

        vkDestroyShaderModule(m_Device.GetVkDevice(), rchitModule, nullptr);
        vkDestroyShaderModule(m_Device.GetVkDevice(), rmissModule, nullptr);
        vkDestroyShaderModule(m_Device.GetVkDevice(), rgenModule, nullptr);
    }

    RayTracingPipeline::~RayTracingPipeline() {
        vkDestroyPipeline(m_Device.GetVkDevice(), m_Pipeline, nullptr);
        vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_PipelineLayout, nullptr);
    }

    std::vector<char> RayTracingPipeline::ReadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    VkShaderModule RayTracingPipeline::CreateShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_Device.GetVkDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }
        return shaderModule;
    }

}
