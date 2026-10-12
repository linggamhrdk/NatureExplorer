#include "Aroxorn/CommandRecorder.hpp"
#include <stdexcept>

namespace Aroxorn {

    CommandRecorder::CommandRecorder(Device& device, uint32_t queueFamilyIndex) : m_Device(device) {
        CreateCommandPool(queueFamilyIndex);
        AllocateCommandBuffer();
    }

    CommandRecorder::~CommandRecorder() {
        vkDestroyCommandPool(m_Device.GetVkDevice(), m_CommandPool, nullptr);
    }

    void CommandRecorder::CreateCommandPool(uint32_t queueFamilyIndex) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndex;

        if (vkCreateCommandPool(m_Device.GetVkDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool for CommandRecorder!");
        }
    }

    void CommandRecorder::AllocateCommandBuffer() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_Device.GetVkDevice(), &allocInfo, &m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffer for CommandRecorder!");
        }
    }

    void CommandRecorder::Begin() {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }
    }

    void CommandRecorder::End() {
        if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void CommandRecorder::BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent, const VkClearValue* clearValues, uint32_t clearValueCount) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = clearValueCount;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void CommandRecorder::EndRenderPass() {
        vkCmdEndRenderPass(m_CommandBuffer);
    }

    void CommandRecorder::BindGraphicsPipeline(VkPipeline pipeline) {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void CommandRecorder::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandRecorder::BindRayTracingPipeline(VkPipeline pipeline) {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    }

    void CommandRecorder::TraceRays(
        const VkStridedDeviceAddressRegionKHR* raygen,
        const VkStridedDeviceAddressRegionKHR* miss,
        const VkStridedDeviceAddressRegionKHR* hit,
        const VkStridedDeviceAddressRegionKHR* callable,
        uint32_t width, uint32_t height, uint32_t depth) {

        auto vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkCmdTraceRaysKHR");
        if (vkCmdTraceRaysKHR) {
            vkCmdTraceRaysKHR(m_CommandBuffer, raygen, miss, hit, callable, width, height, depth);
        } else {
             throw std::runtime_error("Failed to load vkCmdTraceRaysKHR!");
        }
    }

    void CommandRecorder::BindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets) {
        vkCmdBindDescriptorSets(m_CommandBuffer, bindPoint, layout, firstSet, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
    }

}
