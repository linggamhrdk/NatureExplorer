#pragma once

#include "Aroxorn/Device.hpp"
#include <string>
#include <vector>

namespace Aroxorn {

    class RayTracingPipeline {
    public:
        RayTracingPipeline(Device& device, const std::string& rgenShader, const std::string& rmissShader, const std::string& rchitShader);
        ~RayTracingPipeline();

        RayTracingPipeline(const RayTracingPipeline&) = delete;
        RayTracingPipeline& operator=(const RayTracingPipeline&) = delete;

        VkPipeline GetVkPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetVkPipelineLayout() const { return m_PipelineLayout; }

    private:
        Device& m_Device;

        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

        std::vector<char> ReadFile(const std::string& filename);
        VkShaderModule CreateShaderModule(const std::vector<char>& code);
    };

}
