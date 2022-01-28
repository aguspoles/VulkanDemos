#pragma once
#include "render/Utilities.h"

namespace prm
{
    class Swapchain
    {
    public:
        Swapchain(RenderContext& renderContext, vk::Extent2D windowExtent);
        Swapchain(RenderContext& renderContext, vk::Extent2D windowExtent, std::shared_ptr<Swapchain> oldSwapchain);

        Swapchain(const Swapchain&) = delete;

        Swapchain(Swapchain&& other) = delete;

        ~Swapchain();

        Swapchain& operator=(const Swapchain&) = delete;

        Swapchain& operator=(Swapchain&&) = delete;

        vk::SwapchainKHR GetHandle() const { return m_Handle; }

        vk::Result AcquireNextImage(uint32_t& image);

        vk::Result SubmitCommandBuffers(const vk::CommandBuffer buffers, uint32_t imageIndex);

        vk::Format GetImageFormat() const { return m_SwapchainImageFormat; }

        uint32_t GetImagesCount() const { return static_cast<uint32_t>(m_ColorImages.size()); }

        vk::Framebuffer GetFrameBuffer(uint32_t imageIndex) const { return m_FrameBuffers[imageIndex]; }

        vk::Extent2D GetExtent() const { return m_SwapchainExtent; }

        vk::RenderPass GetRenderPass() const { return m_RenderPass; }

    private:
        void Init(vk::Extent2D windowExtent);

        SwapchainDetails GetSwapchainDetails(const vk::PhysicalDevice& gpu) const;
        vk::SurfaceFormatKHR ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
        vk::PresentModeKHR ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>& available_present_modes);
        vk::Extent2D ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR capabilities, vk::Extent2D windowExtent);
        vk::Format FindDepthFormat() const;

        vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect);
        void CreateFrameBuffers();
        void CreateRenderPass();
        void CreateDepthResources();
        void CreateSyncObjects();

        RenderContext& m_RenderContext;
        vk::SwapchainKHR m_Handle;
        std::shared_ptr<Swapchain> m_OldSwapchain;
        vk::Extent2D m_SwapchainExtent;

        //Frame data
        std::vector<vk::Framebuffer> m_FrameBuffers;
        vk::RenderPass m_RenderPass;

        //Color images
        vk::Format m_SwapchainImageFormat;
        std::vector<SwapchainImage> m_ColorImages;

        //Depth images
        vk::Format m_DepthFormat;
        std::vector<SwapchainImage> m_DepthImages;
        std::vector<vk::DeviceMemory> m_DepthImageMemorys;

        //Sync objects
        std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore> m_RenderFinishedSemaphores;

        std::vector<vk::Fence> m_InFlightFences;
        std::vector<vk::Fence> m_ImagesInFlightFences;
        uint32_t m_CurrentFrame = 0;
    };
}


