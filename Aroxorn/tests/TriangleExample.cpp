#include "Aroxorn/Instance.hpp"
#include "Aroxorn/Device.hpp"
#include "Aroxorn/Swapchain.hpp"
#include "Aroxorn/CommandBuffer.hpp"
#include "Aroxorn/Pipeline.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    std::unique_ptr<Aroxorn::Instance> aroxornInstance;
    std::unique_ptr<Aroxorn::Device> aroxornDevice;
    std::unique_ptr<Aroxorn::Swapchain> aroxornSwapchain;
    std::unique_ptr<Aroxorn::Pipeline> aroxornPipeline;
    std::unique_ptr<Aroxorn::CommandBuffer> aroxornCommandBuffer;

    VkSurfaceKHR surface;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Aroxorn Triangle", nullptr, nullptr);
    }

    void initVulkan() {
        aroxornInstance = std::make_unique<Aroxorn::Instance>("Aroxorn App", true);

        if (glfwCreateWindowSurface(aroxornInstance->GetVkInstance(), window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }

        aroxornDevice = std::make_unique<Aroxorn::Device>(*aroxornInstance, surface);
        aroxornSwapchain = std::make_unique<Aroxorn::Swapchain>(*aroxornDevice, surface, WIDTH, HEIGHT);

        createRenderPass();
        createFramebuffers();

        aroxornPipeline = std::make_unique<Aroxorn::Pipeline>(*aroxornDevice, "../shaders/vert.spv", "../shaders/frag.spv", aroxornSwapchain->GetExtent(), renderPass);

        aroxornCommandBuffer = std::make_unique<Aroxorn::CommandBuffer>(*aroxornDevice);

        createSyncObjects();
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = aroxornSwapchain->GetImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(aroxornDevice->GetVkDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(aroxornSwapchain->GetImageViews().size());

        for (size_t i = 0; i < aroxornSwapchain->GetImageViews().size(); i++) {
            VkImageView attachments[] = {
                aroxornSwapchain->GetImageViews()[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = aroxornSwapchain->GetExtent().width;
            framebufferInfo.height = aroxornSwapchain->GetExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(aroxornDevice->GetVkDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(aroxornDevice->GetVkDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(aroxornDevice->GetVkDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(aroxornDevice->GetVkDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = aroxornSwapchain->GetExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aroxornPipeline->GetVkPipeline());

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame() {
        vkWaitForFences(aroxornDevice->GetVkDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(aroxornDevice->GetVkDevice(), 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(aroxornDevice->GetVkDevice(), aroxornSwapchain->GetVkSwapchain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(aroxornCommandBuffer->GetVkCommandBuffer(), 0);
        recordCommandBuffer(aroxornCommandBuffer->GetVkCommandBuffer(), imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        VkCommandBuffer cBuffer = aroxornCommandBuffer->GetVkCommandBuffer();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cBuffer;

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(aroxornDevice->GetGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {aroxornSwapchain->GetVkSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(aroxornDevice->GetPresentQueue(), &presentInfo);
    }

    void mainLoop() {
        // Draw 10 frames as a proof of concept since we are in a headless environment
        // and cannot open an actual window without crashing or freezing.
        std::cout << "Rendering 10 frames..." << std::endl;
        for(int i = 0; i < 10; ++i) {
            glfwPollEvents();
            drawFrame();
        }
        std::cout << "Successfully rendered 10 frames." << std::endl;
        vkDeviceWaitIdle(aroxornDevice->GetVkDevice());
    }

    void cleanup() {
        vkDestroySemaphore(aroxornDevice->GetVkDevice(), renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(aroxornDevice->GetVkDevice(), imageAvailableSemaphore, nullptr);
        vkDestroyFence(aroxornDevice->GetVkDevice(), inFlightFence, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(aroxornDevice->GetVkDevice(), framebuffer, nullptr);
        }

        vkDestroyRenderPass(aroxornDevice->GetVkDevice(), renderPass, nullptr);

        aroxornCommandBuffer.reset();
        aroxornPipeline.reset();
        aroxornSwapchain.reset();
        aroxornDevice.reset();

        vkDestroySurfaceKHR(aroxornInstance->GetVkInstance(), surface, nullptr);

        aroxornInstance.reset();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
