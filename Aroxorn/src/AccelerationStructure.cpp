#include "Aroxorn/AccelerationStructure.hpp"
#include <stdexcept>

namespace Aroxorn {

    AccelerationStructure::AccelerationStructure(Device& device, VkAccelerationStructureTypeKHR type, VkDeviceSize size)
        : m_Device(device) {

        CreateBuffer(
            size,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer = m_Buffer;
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
        vkFreeMemory(m_Device.GetVkDevice(), m_Memory, nullptr);
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

        VkMemoryAllocateFlagsInfo allocFlagsInfo{};
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
        allocInfo.pNext = &allocFlagsInfo;

        if (vkAllocateMemory(m_Device.GetVkDevice(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate acceleration structure buffer memory!");
        }

        vkBindBufferMemory(m_Device.GetVkDevice(), m_Buffer, m_Memory, 0);
    }

    uint32_t AccelerationStructure::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Device.GetVkPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

}
