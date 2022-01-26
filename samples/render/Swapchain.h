#pragma once
#include "render/Utilities.h"

namespace prm
{
    class Swapchain
    {
    public:
        Swapchain(RenderContext& renderContext, vk::Extent2D windowExtent);

        Swapchain(const Swapchain&) = delete;

        Swapchain(Swapchain&& other) = delete;

        ~Swapchain();

        Swapchain& operator=(const Swapchain&) = delete;

        Swapchain& operator=(Swapchain&&) = delete;

        vk::SwapchainKHR GetHandle() const { return m_Handle; }

        void InitFrameBuffers(vk::RenderPass renderPass);

        vk::Result AcquireNextImage(uint32_t& image);

        vk::Result SubmitCommandBuffers(const vk::CommandBuffer* buffers, uint32_t imageIndex);

        vk::Format GetImageFormat() const { return m_SwapchainImageFormat; }

        uint32_t GetImagesCount() const { return static_cast<uint32_t>(m_SwapchainImages.size()); }

        vk::Framebuffer GetFrameBuffer(uint32_t imageIndex) const { return m_FrameBuffers[imageIndex]; }

        vk::Extent2D GetExtent() const { return m_SwapchainExtent; }

    private:
        SwapchainDetails GetSwapchainDetails(const vk::PhysicalDevice& gpu) const;
        vk::SurfaceFormatKHR ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
        vk::PresentModeKHR ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>& available_present_modes);
        vk::Extent2D ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR capabilities, vk::Extent2D windowExtent);

        vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect);
        void CreateFrameBuffers();
        void CreateSyncObjects();

        RenderContext& m_RenderContext;
        vk::SwapchainKHR m_Handle;

        vk::Format m_SwapchainImageFormat;
        vk::Extent2D m_SwapchainExtent;
        std::vector<SwapchainImage> m_SwapchainImages;
        std::vector<vk::Framebuffer> m_FrameBuffers;

        std::vector<vk::Semaphore> imageAvailableSemaphores;
        std::vector<vk::Semaphore> renderFinishedSemaphores;

        std::vector<vk::Fence> inFlightFences;
        std::vector<vk::Fence> imagesInFlight;
        uint32_t currentFrame = 0;
    };
}


