#pragma once
#include "core/glm_defs.h"

namespace prm {
    class CommandPool;
    class Buffer;
    struct RenderContext;

    class Mesh {
    public:
        struct Vertex
        {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};

            static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions();
            static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex& other) const
            {
                return position == other.position && color == other.color && normal == other.normal &&
                    uv == other.uv;
            }
        };

        struct Builder
        {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void loadModel(const std::string& filepath);
        };

        Mesh(RenderContext& renderContext, CommandPool& commandPool, const Mesh::Builder& builder);
        Mesh(RenderContext& renderContext, CommandPool& commandPool, const std::vector<Vertex>& vertices);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        static std::shared_ptr<Mesh> CreateModelFromFile(
            RenderContext& renderContext, CommandPool& commandPool, const std::string& filepath);

        void BindToRenderCommandBuffer(vk::CommandBuffer commandBuffer) const;
        void DrawToRenderCommandBuffer(vk::CommandBuffer commandBuffer) const;

    private:
        void CreateVertexBuffer(const std::vector<Vertex>& vertices);
        void CreateIndexBuffer(const std::vector<uint32_t>& indices);

        RenderContext& m_RenderContext;
        CommandPool& m_CommandPool;

        std::shared_ptr<Buffer> m_VertexBuffer;
        uint32_t m_VertexCount;

        bool m_HasIndexBuffer = false;
        std::shared_ptr<Buffer> m_IndexBuffer;
        uint32_t m_IndexCount;
    };
}

