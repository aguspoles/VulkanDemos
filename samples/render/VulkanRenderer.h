#pragma once
#include "platform/Window.h"
#include "render/Utilities.h"
#include "core/Error.h"

namespace prm
{
    class VulkanRenderer
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        void Init(Window& window);
    private:
        vk::Instance m_Instance;
        vk::PhysicalDevice m_GPU;
        vk::SurfaceKHR m_Surface;
        vk::Device m_LogicalDevice;
        vk::SwapchainKHR m_Swapchain;
        vk::Queue m_GraphicsQueue;
        vk::Queue m_PresentationQueue;
        QueueFamilyIndices m_QueueFamilyIndices;
        vk::Format m_SwapchainImageFormat;
        vk::Extent2D m_SwapchainExtent;
        std::vector<SwapchainImage> m_SwapchainImages;
        std::vector<const char*> m_EnabledInstanceExtensions;
        std::vector<const char*> m_EnabledDeviceExtensions;
#if defined(VKB_DEBUG)
        DebugInfo m_DebugInfo;
#endif


        void CreateInstance(const std::vector<const char*>& requiredInstanceExtensions);
        void CheckInstanceExtensionsSupport(const std::vector<const char*>& required_extensions);

        void FindPhysicalDevice();

        void CreateLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions);
        void CheckDeviceExtensionsSupport(const std::vector<const char*>& required_extensions);

        void CreateSwapchain(Window::Extent windowExtent);
        vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect);

        QueueFamilyIndices GetQueueFamilyIndices(const vk::PhysicalDevice& gpu) const;
        SwapchainDetails GetSwapchainDetails(const vk::PhysicalDevice& gpu) const;
        vk::SurfaceFormatKHR ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
        vk::PresentModeKHR ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>& available_present_modes);
        vk::Extent2D ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR capabilities, Window::Extent windowExtent);
    };
}


