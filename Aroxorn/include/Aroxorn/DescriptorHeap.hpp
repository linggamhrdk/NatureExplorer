#pragma once

#include "Aroxorn/Device.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace Aroxorn {

    class DescriptorHeap {
    public:
        DescriptorHeap(Device& device, uint32_t maxSets = 10);
        ~DescriptorHeap();

        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(const DescriptorHeap&) = delete;

        VkDescriptorPool GetVkDescriptorPool() const { return m_DescriptorPool; }
        VkDescriptorSetLayout GetBindlessLayout() const { return m_BindlessLayout; }

        VkDescriptorSet AllocateSet();

    private:
        Device& m_Device;

        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_BindlessLayout = VK_NULL_HANDLE;

        void CreateDescriptorPool(uint32_t maxSets);
        void CreateBindlessLayout();
    };

}
