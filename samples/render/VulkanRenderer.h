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
    class GameObject;
    class Camera;
    class Buffer;

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
        void Draw(const std::vector<GameObject>& gameObjects, const Camera& camera);
        void RecreateSwapchain();

        void SetVertexShader(const std::string& filePath) { m_VertexShaderPath = filePath; }
        void SetFragmentShader(const std::string& filePath) { m_FragmentShaderPath = filePath; }

        const RenderContext& GetRenderContext() const { return m_RenderContext; }
        RenderContext& GetRenderContext() { return m_RenderContext; }
        CommandPool& GetCommandPool() { return *m_GraphicsCommandPool; }

        float GetAspectRatio() const;

    private:
        Platform& m_Platform;
        RenderContext m_RenderContext;

        vk::PipelineLayout m_PipeLayout;
        vk::PipelineCache m_PipeCache;
        PipelineState m_PipelineState;
        std::vector<ShaderInfo> m_ShaderInfos;

        vk::DescriptorPool m_DescriptorPool;
        std::vector<vk::DescriptorSet> m_DescriptorSets;
        std::vector<vk::DescriptorSetLayout> m_DescriptoSetLayouts;
        
        std::vector<const char*> m_EnabledInstanceExtensions;
        std::vector<const char*> m_EnabledDeviceExtensions;

        std::unique_ptr<Swapchain> m_Swapchain{nullptr};
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline{nullptr};
        std::unique_ptr<CommandPool> m_GraphicsCommandPool{ nullptr };

        std::string m_VertexShaderPath;
        std::string m_FragmentShaderPath;

        std::shared_ptr<Buffer> m_ShaderUniformBuffer;

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

        void CreateDescriptorSets();

        void CreatePipelineLayout();

        void RecordCommandBuffer(uint32_t index, const std::vector<GameObject>& gameObjects, const Camera& camera) const;

        void SetViewportAndScissor(vk::CommandBuffer buffer) const;

        void Render(uint32_t index, const std::vector<GameObject>& gameObjects, const Camera& camera);
    };
}


