#pragma once
#include "platform/Window.h"
#include "render/Utilities.h"
#include "core/Error.h"

namespace prm
{
    class GraphicsPipeline;
    class Swapchain;
    class CommandPool;

    class VulkanRenderer
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        void Init(Window& window);
        void Draw(Window::Extent windowExtent);
        void RecreateSwapchain(Window::Extent windowExtent);

    private:
        vk::Instance m_Instance;
        vk::PhysicalDevice m_GPU;
        vk::SurfaceKHR m_Surface;
        vk::Device m_LogicalDevice;

        vk::Queue m_GraphicsQueue;
        vk::Queue m_PresentationQueue;
        QueueFamilyIndices m_QueueFamilyIndices;

        vk::PipelineLayout m_PipeLayout;
        vk::RenderPass m_RenderPass;
        vk::PipelineCache m_PipeCache;
        
        std::vector<const char*> m_EnabledInstanceExtensions;
        std::vector<const char*> m_EnabledDeviceExtensions;

        std::unique_ptr<Swapchain> m_Swapchain{nullptr};
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline{nullptr};
        std::unique_ptr<CommandPool> m_CommandPool{ nullptr };

#if defined(VKB_DEBUG)
        DebugInfo m_DebugInfo;
#endif
        void CreateInstance(const std::vector<const char*>& requiredInstanceExtensions);
        void CheckInstanceExtensionsSupport(const std::vector<const char*>& required_extensions);

        void FindPhysicalDevice();

        void CreateLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions);
        void CheckDeviceExtensionsSupport(const std::vector<const char*>& required_extensions);

        QueueFamilyIndices GetQueueFamilyIndices(const vk::PhysicalDevice& gpu) const;

        vk::Result PresentImage(uint32_t index);

        void CreateRenderPass();

        void RecordCommandBuffer(uint32_t index) const;

        void SetViewportAndSissor(vk::CommandBuffer buffer) const;

        void Render(uint32_t index);
    };
}


