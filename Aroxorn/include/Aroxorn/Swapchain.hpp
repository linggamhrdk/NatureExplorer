#pragma once

#include "Aroxorn/Device.hpp"
#include <vector>

namespace Aroxorn {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class Swapchain {
    public:
        Swapchain(Device& device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        VkSwapchainKHR GetVkSwapchain() const { return m_Swapchain; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }
        const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

        static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    private:
        Device& m_Device;
        VkSurfaceKHR m_Surface;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_Images;
        VkFormat m_ImageFormat;
        VkExtent2D m_Extent;
        std::vector<VkImageView> m_ImageViews;

        void CreateSwapchain(uint32_t width, uint32_t height);
        void CreateImageViews();

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
    };

}
