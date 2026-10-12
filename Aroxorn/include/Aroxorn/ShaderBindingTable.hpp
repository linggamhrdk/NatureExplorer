#pragma once

#include "Aroxorn/Device.hpp"
#include "Aroxorn/RayTracingPipeline.hpp"
#include "Aroxorn/MemoryAllocator.hpp"

namespace Aroxorn {

    class ShaderBindingTable {
    public:
        ShaderBindingTable(Device& device, MemoryAllocator& allocator, RayTracingPipeline& pipeline);
        ~ShaderBindingTable();

        ShaderBindingTable(const ShaderBindingTable&) = delete;
        ShaderBindingTable& operator=(const ShaderBindingTable&) = delete;

        VkStridedDeviceAddressRegionKHR GetRaygenRegion() const { return m_RaygenRegion; }
        VkStridedDeviceAddressRegionKHR GetMissRegion() const { return m_MissRegion; }
        VkStridedDeviceAddressRegionKHR GetHitRegion() const { return m_HitRegion; }

    private:
        Device& m_Device;
        MemoryAllocator& m_Allocator;

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        Allocation m_Allocation{};

        VkStridedDeviceAddressRegionKHR m_RaygenRegion{};
        VkStridedDeviceAddressRegionKHR m_MissRegion{};
        VkStridedDeviceAddressRegionKHR m_HitRegion{};

        uint32_t AlignUp(uint32_t size, uint32_t alignment);
    };

}
