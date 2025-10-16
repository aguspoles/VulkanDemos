#include "pch.h"
#include "render/Mesh.h"
#include "render/Utilities.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "core/glm_defs.h"
#include "core/Error.h"
#include "render/CommandPool.h"
#include "render/Buffer.h"

namespace std {
    template <>
    struct hash<prm::Mesh::Vertex>
    {
        size_t operator()(prm::Mesh::Vertex const& vertex) const
        {
            size_t seed = 0;
            prm::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}  // namespace std

namespace prm {

    Mesh::Mesh(RenderContext& renderContext, CommandPool& commandPool, const Mesh::Builder& builder)
        : m_RenderContext{ renderContext }
        , m_CommandPool(commandPool)
    {
        CreateVertexBuffer(builder.vertices);
        CreateIndexBuffer(builder.indices);
    }

    Mesh::Mesh(RenderContext& renderContext, CommandPool& commandPool,
        const std::vector<Vertex>& vertices)
            : m_RenderContext{ renderContext }
            , m_CommandPool(commandPool)
    {
        CreateVertexBuffer(vertices);
    }

    Mesh::~Mesh()
    {
        m_VertexBuffer.reset();

        if (m_HasIndexBuffer) 
        {
            m_IndexBuffer.reset();
        }
    }

    std::shared_ptr<Mesh> Mesh::CreateModelFromFile(
        RenderContext& renderContext, CommandPool& commandPool, const std::string& filepath)
    {
        Builder builder{};
        builder.loadModel(filepath);
        LOGI("Loaded model with {} vertices and {} indices", builder.vertices.size(), builder.indices.size());
        return std::make_shared<Mesh>(renderContext, commandPool, builder);
    }

    void Mesh::CreateVertexBuffer(const std::vector<Vertex>& vertices)
    {
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        assert(m_VertexCount >= 3 && "Vertex count must be at least 3");
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;

        m_VertexBuffer = Buffer::CreateBuffer(m_RenderContext, bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);

        m_VertexBuffer->UpdateDeviceData(static_cast<const void*>(vertices.data()), m_CommandPool);
    }

    void Mesh::CreateIndexBuffer(const std::vector<uint32_t>& indices)
    {
        m_IndexCount = static_cast<uint32_t>(indices.size());
        m_HasIndexBuffer = m_IndexCount > 0;

        if (!m_HasIndexBuffer) {
            return;
        }

        const vk::DeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;

        m_IndexBuffer = Buffer::CreateBuffer(m_RenderContext, bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);

        m_IndexBuffer->UpdateDeviceData(static_cast<const void*>(indices.data()), m_CommandPool);
    }

    void Mesh::DrawToRenderCommandBuffer(vk::CommandBuffer commandBuffer) const
    {
        if (m_HasIndexBuffer) 
        {
            commandBuffer.drawIndexed(m_IndexCount, 1, 0, 0, 0);
        }
        else 
        {
            commandBuffer.draw(m_VertexCount, 1, 0, 0);
        }
    }

    void Mesh::BindToRenderCommandBuffer(vk::CommandBuffer commandBuffer) const
    {
        vk::Buffer buffers[] = { m_VertexBuffer->GetDeviceBuffer()};
        vk::DeviceSize offsets[] = { 0 };
        commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);

        if (m_HasIndexBuffer) 
        {
            commandBuffer.bindIndexBuffer(m_IndexBuffer->GetDeviceBuffer(), 0, vk::IndexType::eUint32);
        }
    }

    std::vector<vk::VertexInputBindingDescription> Mesh::Vertex::getBindingDescriptions()
    {
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;
        return bindingDescriptions;
    }

    std::vector<vk::VertexInputAttributeDescription> Mesh::Vertex::getAttributeDescriptions()
    {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions(4);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[3].offset = offsetof(Vertex, uv);
        return attributeDescriptions;
    }

    void Mesh::Builder::loadModel(const std::string& filepath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) 
        {
            throw std::runtime_error(warn + err);
        }

        vertices.clear();
        indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (const auto& shape : shapes) 
        {
            for (const auto& index : shape.mesh.indices) 
            {
                Vertex vertex{};

                if (index.vertex_index >= 0) 
                {
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                    };

                    vertex.color = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2],
                    };
                }

                if (index.normal_index >= 0) 
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }

                if (index.texcoord_index >= 0) 
                {
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (uniqueVertices.count(vertex) == 0) 
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

}