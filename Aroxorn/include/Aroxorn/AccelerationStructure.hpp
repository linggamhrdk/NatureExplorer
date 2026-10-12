#pragma once

#include "Aroxorn/Device.hpp"

namespace Aroxorn {

    class AccelerationStructure {
    public:
        AccelerationStructure(Device& device, VkAccelerationStructureTypeKHR type, VkDeviceSize size);
        ~AccelerationStructure();

        AccelerationStructure(const AccelerationStructure&) = delete;
        AccelerationStructure& operator=(const AccelerationStructure&) = delete;

        VkAccelerationStructureKHR GetVkAccelerationStructure() const { return m_Handle; }
        VkDeviceAddress GetDeviceAddress() const { return m_DeviceAddress; }

    private:
        Device& m_Device;

        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkDeviceAddress m_DeviceAddress = 0;

        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    };

}
