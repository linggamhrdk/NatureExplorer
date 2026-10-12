#pragma once

#include "Aroxorn/Device.hpp"
#include "Aroxorn/MemoryAllocator.hpp"

namespace Aroxorn {

    class AccelerationStructure {
    public:
        AccelerationStructure(Device& device, MemoryAllocator& allocator, VkAccelerationStructureTypeKHR type, VkDeviceSize size);
        ~AccelerationStructure();

        AccelerationStructure(const AccelerationStructure&) = delete;
        AccelerationStructure& operator=(const AccelerationStructure&) = delete;

        VkAccelerationStructureKHR GetVkAccelerationStructure() const { return m_Handle; }
        VkDeviceAddress GetDeviceAddress() const { return m_DeviceAddress; }

    private:
        Device& m_Device;
        MemoryAllocator& m_Allocator;

        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        Allocation m_Allocation{};
        VkDeviceAddress m_DeviceAddress = 0;

        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    };

}
