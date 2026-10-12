#pragma once

#include "Aroxorn/Device.hpp"
#include <string>
#include <vector>

namespace Aroxorn {

    class Pipeline {
    public:
        Pipeline(Device& device, const std::string& vertShaderPath, const std::string& fragShaderPath, VkExtent2D extent, VkRenderPass renderPass);
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        VkPipeline GetVkPipeline() const { return m_GraphicsPipeline; }
        VkPipelineLayout GetVkPipelineLayout() const { return m_PipelineLayout; }

    private:
        Device& m_Device;

        VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

        std::vector<char> ReadFile(const std::string& filename);
        VkShaderModule CreateShaderModule(const std::vector<char>& code);
    };

}
