#pragma once
#include "core/glm_defs.h"

namespace prm {
    class CommandPool;

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

        Mesh(vk::PhysicalDevice gpu, vk::Device device, CommandPool& commandPool, const Mesh::Builder& builder);
        Mesh(vk::PhysicalDevice gpu, vk::Device device, CommandPool& commandPool, const std::vector<Vertex>& vertices);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        static std::shared_ptr<Mesh> CreateModelFromFile(
            vk::PhysicalDevice gpu, vk::Device device, CommandPool& commandPool, const std::string& filepath);

        void Bind(vk::CommandBuffer commandBuffer) const;
        void Draw(vk::CommandBuffer commandBuffer) const;

    private:
        void CreateVertexBuffers(const std::vector<Vertex>& vertices);
        void CreateIndexBuffers(const std::vector<uint32_t>& indices);

        vk::Device m_Device;
        vk::PhysicalDevice m_GPU;
        CommandPool& m_CommandPool;

        vk::Buffer m_VertexBuffer;
        vk::DeviceMemory m_VertexBufferMemory;
        uint32_t m_VertexCount;

        bool m_HasIndexBuffer = false;
        vk::Buffer m_IndexBuffer;
        vk::DeviceMemory m_IndexBufferMemory;
        uint32_t m_IndexCount;
    };
}

