#include "Aroxorn/DescriptorHeap.hpp"
#include <stdexcept>

namespace Aroxorn {

    DescriptorHeap::DescriptorHeap(Device& device, uint32_t maxSets)
        : m_Device(device) {
        CreateDescriptorPool(maxSets);
        CreateBindlessLayout();
    }

    DescriptorHeap::~DescriptorHeap() {
        vkDestroyDescriptorSetLayout(m_Device.GetVkDevice(), m_BindlessLayout, nullptr);
        vkDestroyDescriptorPool(m_Device.GetVkDevice(), m_DescriptorPool, nullptr);
    }

    void DescriptorHeap::CreateDescriptorPool(uint32_t maxSets) {
        // In a true bindless setup, we request massive amounts of descriptors.
        const uint32_t MAX_BINDLESS_RESOURCES = 16384;

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCES},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_RESOURCES},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_BINDLESS_RESOURCES},
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, maxSets} // Usually 1 TLAS per frame
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        if (vkCreateDescriptorPool(m_Device.GetVkDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bindless descriptor pool!");
        }
    }

    void DescriptorHeap::CreateBindlessLayout() {
        const uint32_t MAX_BINDLESS_RESOURCES = 16384;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            // Binding 0: Textures
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCES, VK_SHADER_STAGE_ALL, nullptr},
            // Binding 1: Storage Images
            {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_RESOURCES, VK_SHADER_STAGE_ALL, nullptr},
            // Binding 2: Storage Buffers
            {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_BINDLESS_RESOURCES, VK_SHADER_STAGE_ALL, nullptr},
            // Binding 3: TLAS (Ray Tracing)
            {3, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr}
        };

        // All arrays are partially bound and update-after-bind compatible
        VkDescriptorBindingFlags bindlessFlags =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

        std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size(), bindlessFlags);

        VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlags{};
        layoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        layoutBindingFlags.bindingCount = static_cast<uint32_t>(bindingFlags.size());
        layoutBindingFlags.pBindingFlags = bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.pNext = &layoutBindingFlags;

        if (vkCreateDescriptorSetLayout(m_Device.GetVkDevice(), &layoutInfo, nullptr, &m_BindlessLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bindless descriptor set layout!");
        }
    }

    VkDescriptorSet DescriptorHeap::AllocateSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_BindlessLayout;

        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(m_Device.GetVkDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate bindless descriptor set!");
        }

        return descriptorSet;
    }

}
