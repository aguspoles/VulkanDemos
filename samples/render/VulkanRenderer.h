#pragma once
#include "platform/Platform.h"
#include "render/PipelineState.h"
#include "render/GraphicsPipeline.h"
#include "core/Error.h"

namespace prm
{
    class GraphicsPipeline;
    class Swapchain;
    class CommandPool;
    class Mesh;
    class IRenderableObject;
    class Camera;
    class Buffer;
    class Texture;
    struct RenderContext;

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
        void Finish();
        void PrepareResources();
        void CleanupResources();
        void Draw(const std::vector<IRenderableObject*>& renderableObjects, const Camera& camera);
        void RecreateSwapchain();

        void SetVertexShader(const std::string& filePath) { m_VertexShaderPath = filePath; }
        void SetFragmentShader(const std::string& filePath) { m_FragmentShaderPath = filePath; }
        void AddTexture(const std::shared_ptr<Texture>& texture);

        const RenderContext& GetRenderContext() const;
        RenderContext& GetRenderContext();
        CommandPool& GetCommandPool() { return *m_GraphicsCommandPool; }

        float GetAspectRatio() const;

    private:
        Platform& m_Platform;
        std::unique_ptr<RenderContext> m_RenderContext;

        vk::PipelineLayout m_PipeLayout{};
        vk::PipelineCache m_PipeCache{};
        PipelineState m_PipelineState;
        std::vector<ShaderInfo> m_ShaderInfos;

        vk::DescriptorPool m_DescriptorPool{};
        std::vector<vk::DescriptorSet> m_DescriptorSets;
        std::vector<vk::DescriptorSetLayout> m_DescriptoSetLayouts;

        std::unique_ptr<Swapchain> m_Swapchain{nullptr};
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline{nullptr};
        std::unique_ptr<CommandPool> m_GraphicsCommandPool{ nullptr };

        std::string m_VertexShaderPath;
        std::string m_FragmentShaderPath;

        std::vector<std::shared_ptr<Buffer>> m_PerFrameUniformBuffers;
        std::vector<std::shared_ptr<Texture>> m_Textures;

        void CreateUniformBuffers();

        void CreateSwapchain();

        void CreateDescriptorSets();

        void CreatePipelineLayout();

        void RecordCommandBuffer(uint32_t index, const std::vector<IRenderableObject*>& renderableObjects, const Camera& camera) const;

        void SetViewportAndScissor(vk::CommandBuffer buffer) const;

        void Render(uint32_t index, const std::vector<IRenderableObject*>& renderableObjects, const Camera& camera);
    };
}


