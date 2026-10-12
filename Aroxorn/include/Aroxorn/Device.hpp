#pragma once

#include "Aroxorn/Instance.hpp"
#include <optional>

namespace Aroxorn {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class Device {
    public:
        Device(Instance& instance, VkSurfaceKHR surface);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        VkDevice GetVkDevice() const { return m_LogicalDevice; }
        VkPhysicalDevice GetVkPhysicalDevice() const { return m_PhysicalDevice; }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

    private:
        Instance& m_Instance;
        VkSurfaceKHR m_Surface;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_LogicalDevice = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        void PickPhysicalDevice();
        void CreateLogicalDevice();

        bool IsDeviceSuitable(VkPhysicalDevice device);
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    };

}
