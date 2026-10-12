#pragma once

#include "Aroxorn/Device.hpp"
#include <vector>

namespace Aroxorn {

    class CommandBuffer {
    public:
        CommandBuffer(Device& device);
        ~CommandBuffer();

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        void Begin();
        void End();

        VkCommandBuffer GetVkCommandBuffer() const { return m_CommandBuffer; }

    private:
        Device& m_Device;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

        void CreateCommandPool();
        void AllocateCommandBuffers();
    };

}
