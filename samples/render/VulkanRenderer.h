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
        vk::Queue m_GraphicsQueue;
        std::vector<const char*> m_EnabledExtensions;
#if defined(VKB_DEBUG)
        DebugInfo m_DebugInfo;
#endif


        void CreateInstance(Window& window);
        void CheckInstanceExtensionsSupport(const std::vector<const char*>& required_extensions);

        void FindPhysicalDevice();
        vk::Bool32 IsPresentSupported(vk::PhysicalDevice gpu, vk::SurfaceKHR surface, uint32_t queue_family_index) const;
        QueueFamilyIndices GetQueueFamilyIndices(const vk::PhysicalDevice& gpu) const;

        void CreateLogicalDevice();

    };
}


