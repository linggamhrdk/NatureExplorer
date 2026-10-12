#include "Aroxorn/AccelerationStructure.hpp"
#include <stdexcept>

namespace Aroxorn {

    AccelerationStructure::AccelerationStructure(Device& device, MemoryAllocator& allocator, VkAccelerationStructureTypeKHR type, VkDeviceSize size)
        : m_Device(device), m_Allocator(allocator) {

        CreateBuffer(
            size,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer = m_Buffer;
        createInfo.offset = m_Allocation.offset;
        createInfo.size = size;
        createInfo.type = type;

        auto vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkCreateAccelerationStructureKHR");
        if (!vkCreateAccelerationStructureKHR) {
            throw std::runtime_error("Failed to load vkCreateAccelerationStructureKHR extension!");
        }

        if (vkCreateAccelerationStructureKHR(m_Device.GetVkDevice(), &createInfo, nullptr, &m_Handle) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create acceleration structure!");
        }

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = m_Handle;

        auto vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkGetAccelerationStructureDeviceAddressKHR");

        if (vkGetAccelerationStructureDeviceAddressKHR) {
            m_DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(m_Device.GetVkDevice(), &addressInfo);
        } else {
             throw std::runtime_error("Failed to load vkGetAccelerationStructureDeviceAddressKHR extension!");
        }
    }

    AccelerationStructure::~AccelerationStructure() {
        auto vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(m_Device.GetVkDevice(), "vkDestroyAccelerationStructureKHR");
        if (vkDestroyAccelerationStructureKHR && m_Handle != VK_NULL_HANDLE) {
            vkDestroyAccelerationStructureKHR(m_Device.GetVkDevice(), m_Handle, nullptr);
        }

        vkDestroyBuffer(m_Device.GetVkDevice(), m_Buffer, nullptr);

        // Free via Aroxorn custom allocator
        m_Allocator.Free(m_Allocation);
    }

    void AccelerationStructure::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_Device.GetVkDevice(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create acceleration structure buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device.GetVkDevice(), m_Buffer, &memRequirements);

        // Sub-allocate via Aroxorn Free-List (false for useLinear, true for threadSafe)
        m_Allocation = m_Allocator.Allocate(memRequirements, properties, false, true);

        // Bind the sub-allocated chunk using offset
        vkBindBufferMemory(m_Device.GetVkDevice(), m_Buffer, m_Allocation.memory, m_Allocation.offset);
    }

}
