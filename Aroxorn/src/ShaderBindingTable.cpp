#include "Aroxorn/ShaderBindingTable.hpp"
#include <stdexcept>
#include <vector>
#include <cstring>

namespace Aroxorn {

    ShaderBindingTable::ShaderBindingTable(Device& device, MemoryAllocator& allocator, RayTracingPipeline& pipeline)
        : m_Device(device), m_Allocator(allocator) {

        // Query RT properties for alignment
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{};
        rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 properties2{};
        properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties2.pNext = &rtProperties;

        vkGetPhysicalDeviceProperties2(m_Device.GetVkPhysicalDevice(), &properties2);

        uint32_t handleSize = rtProperties.shaderGroupHandleSize;
        uint32_t handleSizeAligned = AlignUp(handleSize, rtProperties.shaderGroupHandleAlignment);
        uint32_t groupCount = 3; // Raygen, Miss, Hit

        uint32_t sbtSize = groupCount * handleSizeAligned;

        auto vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkGetRayTracingShaderGroupHandlesKHR");

        if (!vkGetRayTracingShaderGroupHandlesKHR) {
            throw std::runtime_error("Failed to load vkGetRayTracingShaderGroupHandlesKHR!");
        }

        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        if (vkGetRayTracingShaderGroupHandlesKHR(m_Device.GetVkDevice(), pipeline.GetVkPipeline(), 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get shader group handles!");
        }

        // Create Buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sbtSize;
        bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_Device.GetVkDevice(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create SBT buffer!");
        }

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_Device.GetVkDevice(), m_Buffer, &memReqs);

        // Sub-allocate using Linear (Bump) Allocator since SBT is built once per pipeline, requires host visibility
        m_Allocation = m_Allocator.Allocate(memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, true);

        vkBindBufferMemory(m_Device.GetVkDevice(), m_Buffer, m_Allocation.memory, m_Allocation.offset);

        // Copy handles into mapped memory
        if (m_Allocation.mappedData == nullptr) {
            throw std::runtime_error("Linear Allocator memory must be host visible for SBT mapping!");
        }

        uint8_t* pData = static_cast<uint8_t*>(m_Allocation.mappedData);
        for (uint32_t i = 0; i < groupCount; i++) {
            memcpy(pData, shaderHandleStorage.data() + i * handleSize, handleSize);
            pData += handleSizeAligned;
        }

        // Setup Regions
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = m_Buffer;

        VkDeviceAddress sbtAddress = 0;
        auto vkGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkGetBufferDeviceAddress");
        if (vkGetBufferDeviceAddress) {
            sbtAddress = vkGetBufferDeviceAddress(m_Device.GetVkDevice(), &info);
        }

        m_RaygenRegion.deviceAddress = sbtAddress;
        m_RaygenRegion.stride = handleSizeAligned;
        m_RaygenRegion.size = handleSizeAligned;

        m_MissRegion.deviceAddress = sbtAddress + handleSizeAligned;
        m_MissRegion.stride = handleSizeAligned;
        m_MissRegion.size = handleSizeAligned;

        m_HitRegion.deviceAddress = sbtAddress + handleSizeAligned * 2;
        m_HitRegion.stride = handleSizeAligned;
        m_HitRegion.size = handleSizeAligned;
    }

    ShaderBindingTable::~ShaderBindingTable() {
        vkDestroyBuffer(m_Device.GetVkDevice(), m_Buffer, nullptr);
        // Note: Linear allocations are not freed individually.
        // They reset globally at the end of the frame/lifecycle.
    }

    uint32_t ShaderBindingTable::AlignUp(uint32_t size, uint32_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

}
