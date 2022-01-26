#pragma once
#include "platform/Platform.h"
#include "render/Utilities.h"
#include "render/PipelineState.h"
#include "core/Error.h"

namespace prm
{
    class GraphicsPipeline;
    class Swapchain;
    class CommandPool;
    class Mesh;

    class VulkanRenderer
    {
    public:
        VulkanRenderer(Platform& platform);
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&) = delete;

        VulkanRenderer(VulkanRenderer&&) = delete;

        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        void Init();
        void Draw(const Mesh& mesh);
        void RecreateSwapchain();

        const RenderContext& GetRenderContext() const { return m_RenderContext; }

    private:
        Platform& m_Platform;
        RenderContext m_RenderContext;

        vk::PipelineLayout m_PipeLayout;
        vk::PipelineCache m_PipeCache;
        PipelineState m_PipelineState;
        std::vector<ShaderInfo> m_ShaderInfos;
        
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

        void CreateSwapchain();

        void CreatePipelineLayout();

        void RecordCommandBuffer(uint32_t index, const Mesh& mesh) const;

        void SetViewportAndScissor(vk::CommandBuffer buffer) const;

        void Render(uint32_t index, const Mesh& mesh);
    };
}


