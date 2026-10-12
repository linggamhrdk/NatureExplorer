#include "Aroxorn/CommandBuffer.hpp"
#include <stdexcept>

namespace Aroxorn {

    CommandBuffer::CommandBuffer(Device& device) : m_Device(device) {
        CreateCommandPool();
        AllocateCommandBuffers();
    }

    CommandBuffer::~CommandBuffer() {
        vkDestroyCommandPool(m_Device.GetVkDevice(), m_CommandPool, nullptr);
    }

    void CommandBuffer::CreateCommandPool() {
        QueueFamilyIndices queueFamilyIndices = m_Device.FindQueueFamilies(m_Device.GetVkPhysicalDevice());

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(m_Device.GetVkDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void CommandBuffer::AllocateCommandBuffers() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_Device.GetVkDevice(), &allocInfo, &m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void CommandBuffer::Begin() {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }
    }

    void CommandBuffer::End() {
        if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

}
