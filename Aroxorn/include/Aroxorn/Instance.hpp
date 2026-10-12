#pragma once

#include "Aroxorn/Types.hpp"
#include <vector>
#include <memory>
#include <string>

namespace Aroxorn {

    class Instance {
    public:
        Instance(const std::string& appName, bool enableValidationLayers = true);
        ~Instance();

        // Prevent copying to maintain unique ownership of Vulkan handles
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        VkInstance GetVkInstance() const { return m_Instance; }

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

        bool m_EnableValidationLayers;

        void CreateInstance(const std::string& appName);
        void SetupDebugMessenger();

        bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
        std::vector<const char*> GetRequiredExtensions();
    };

}
