#pragma once

#include "Aroxorn/Device.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace Aroxorn {

    class CommandRecorder {
    public:
        CommandRecorder(Device& device, uint32_t queueFamilyIndex);
        ~CommandRecorder();

        CommandRecorder(const CommandRecorder&) = delete;
        CommandRecorder& operator=(const CommandRecorder&) = delete;

        void Begin();
        void End();

        // Standard Graphics
        void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent, const VkClearValue* clearValues, uint32_t clearValueCount);
        void EndRenderPass();
        void BindGraphicsPipeline(VkPipeline pipeline);
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

        // Ray Tracing
        void BindRayTracingPipeline(VkPipeline pipeline);
        void TraceRays(
            const VkStridedDeviceAddressRegionKHR* raygen,
            const VkStridedDeviceAddressRegionKHR* miss,
            const VkStridedDeviceAddressRegionKHR* hit,
            const VkStridedDeviceAddressRegionKHR* callable,
            uint32_t width, uint32_t height, uint32_t depth
        );

        // Bindless Descriptors
        void BindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets);

        VkCommandBuffer GetVkCommandBuffer() const { return m_CommandBuffer; }

    private:
        Device& m_Device;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

        void CreateCommandPool(uint32_t queueFamilyIndex);
        void AllocateCommandBuffer();
    };

}
