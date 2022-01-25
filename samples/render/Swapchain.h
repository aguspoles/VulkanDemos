#pragma once
#include "render/Utilities.h"

namespace prm
{
    class Swapchain
    {
    public:
        Swapchain(vk::Device device, vk::PhysicalDevice gpu, vk::SurfaceKHR surface, const QueueFamilyIndices& queueFamilyIndices, vk::Extent2D windowExtent);

        Swapchain(const Swapchain&) = delete;

        Swapchain(Swapchain&& other) = delete;

        ~Swapchain();

        Swapchain& operator=(const Swapchain&) = delete;

        Swapchain& operator=(Swapchain&&) = delete;

        vk::SwapchainKHR GetHandle() const { return m_Handle; }

        void InitFrameBuffers(vk::RenderPass renderPass);

        vk::Result AcquireNextImage(uint32_t& image);

        vk::Format GetImageFormat() const { return m_SwapchainImageFormat; }

        uint32_t GetImagesCount() const { return static_cast<uint32_t>(m_SwapchainImages.size()); }

        vk::Framebuffer GetFrameBuffer(uint32_t imageIndex) const { return m_FrameBuffers[imageIndex]; }

        vk::Extent2D GetExtent() const { return m_SwapchainExtent; }

        vk::Semaphore GetRenderSemaphore(uint32_t imageIndex) const;
        vk::Semaphore GetPresentSemaphore(uint32_t imageIndex) const;
        vk::Fence GetFence(uint32_t imageIndex) const;

    private:
        SwapchainDetails GetSwapchainDetails(const vk::PhysicalDevice& gpu) const;
        vk::SurfaceFormatKHR ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
        vk::PresentModeKHR ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>& available_present_modes);
        vk::Extent2D ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR capabilities, vk::Extent2D windowExtent);
        vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect);
        void CreateFrameBuffers();

        vk::Device m_LogicalDevice;
        vk::PhysicalDevice m_GPU;
        vk::SurfaceKHR m_Surface;
        vk::SwapchainKHR m_Handle;

        vk::Format m_SwapchainImageFormat;
        vk::Extent2D m_SwapchainExtent;
        std::vector<SwapchainImage> m_SwapchainImages;
        std::vector<vk::Framebuffer> m_FrameBuffers;
        QueueFamilyIndices m_QueueFamilyIndices{};

        struct Semaphores
        {
            vk::Semaphore renderSemaphore;
            vk::Semaphore presentSemaphore;
        };
        std::vector<Semaphores> m_RecycledSemaphores;
        std::vector<Semaphores> m_SemaphoreByImageIndex;

        std::vector<vk::Fence> m_Fences;
    };
}


