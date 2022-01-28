#include "pch.h"
#include "render/Mesh.h"
#include "render/Utilities.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "core/glm_defs.h"
#include "core/Error.h"
#include "render/CommandPool.h"

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

    Mesh::Mesh(vk::PhysicalDevice gpu, vk::Device device, CommandPool& commandPool, const Mesh::Builder& builder)
        : m_Device{ device }
        , m_GPU(gpu)
        , m_CommandPool(commandPool)
    {
        CreateVertexBuffers(builder.vertices);
        CreateIndexBuffers(builder.indices);
    }

    Mesh::Mesh(vk::PhysicalDevice gpu, vk::Device device, CommandPool& commandPool,
        const std::vector<Vertex>& vertices)
            : m_Device{ device }
            , m_GPU(gpu)
            , m_CommandPool(commandPool)
    {
        CreateVertexBuffers(vertices);
    }

    Mesh::~Mesh()
    {
        m_Device.destroyBuffer(m_VertexBuffer);
        m_Device.freeMemory(m_VertexBufferMemory);

        if (m_HasIndexBuffer) 
        {
            m_Device.destroyBuffer(m_IndexBuffer);
            m_Device.freeMemory(m_IndexBufferMemory);
        }
    }

    std::shared_ptr<Mesh> Mesh::CreateModelFromFile(
        vk::PhysicalDevice gpu, vk::Device device, CommandPool& commandPool, const std::string& filepath)
    {
        Builder builder{};
        builder.loadModel(filepath);
        LOGI("Loaded model with {} vertices and {} indices", builder.vertices.size(), builder.indices.size());
        return std::make_shared<Mesh>(gpu, device, commandPool, builder);
    }

    void Mesh::CreateVertexBuffers(const std::vector<Vertex>& vertices)
    {
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        assert(m_VertexCount >= 3 && "Vertex count must be at least 3");
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;

        //Staging buffer set up
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_Device.createBuffer(&bufferInfo, nullptr, &stagingBuffer));

        vk::MemoryRequirements memRequirements;
        m_Device.getBufferMemoryRequirements(stagingBuffer, &memRequirements);

        auto hostVisible = vk::MemoryPropertyFlagBits::eHostVisible;
        auto coherent = vk::MemoryPropertyFlagBits::eHostCoherent;
        auto mask = static_cast<vk::MemoryPropertyFlagBits> (
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(hostVisible) |
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(coherent)
            );

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_GPU.getMemoryProperties(), mask);

        VK_CHECK(m_Device.allocateMemory(&allocInfo, nullptr, &stagingBufferMemory));

        m_Device.bindBufferMemory(stagingBuffer, stagingBufferMemory, 0);

        void* data;
        VK_CHECK(m_Device.mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data));
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        m_Device.unmapMemory(stagingBufferMemory);

        //Vertex buffer set up

        vk::BufferCreateInfo vertexBufferInfo;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        vertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_Device.createBuffer(&vertexBufferInfo, nullptr, &m_VertexBuffer));

        vk::MemoryRequirements vertexMemRequirements;
        m_Device.getBufferMemoryRequirements(m_VertexBuffer, &vertexMemRequirements);

        vk::MemoryAllocateInfo vertexAllocInfo{};
        vertexAllocInfo.allocationSize = vertexMemRequirements.size;
        vertexAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(vertexMemRequirements.memoryTypeBits, m_GPU.getMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal);

        VK_CHECK(m_Device.allocateMemory(&vertexAllocInfo, nullptr, &m_VertexBufferMemory));

        m_Device.bindBufferMemory(m_VertexBuffer, m_VertexBufferMemory, 0);

        //Copy data between buffers
        vk::CommandBuffer commandBuffer = m_CommandPool.BeginOneTimeSubmitCommand();

        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = bufferSize;
        commandBuffer.copyBuffer(stagingBuffer, m_VertexBuffer, 1, &copyRegion);

        m_CommandPool.EndOneTimeSubmitCommand(commandBuffer);

        //Finally we can destroy the staging buffer
        m_Device.destroyBuffer(stagingBuffer);
        m_Device.freeMemory(stagingBufferMemory);
    }

    void Mesh::CreateIndexBuffers(const std::vector<uint32_t>& indices)
    {
        m_IndexCount = static_cast<uint32_t>(indices.size());
        m_HasIndexBuffer = m_IndexCount > 0;

        if (!m_HasIndexBuffer) {
            return;
        }

        vk::DeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;

        //Staging buffer set up
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_Device.createBuffer(&bufferInfo, nullptr, &stagingBuffer));

        vk::MemoryRequirements memRequirements;
        m_Device.getBufferMemoryRequirements(stagingBuffer, &memRequirements);

        auto hostVisible = vk::MemoryPropertyFlagBits::eHostVisible;
        auto coherent = vk::MemoryPropertyFlagBits::eHostCoherent;
        auto mask = static_cast<vk::MemoryPropertyFlagBits> (
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(hostVisible) |
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(coherent)
            );

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_GPU.getMemoryProperties(), mask);

        VK_CHECK(m_Device.allocateMemory(&allocInfo, nullptr, &stagingBufferMemory));

        m_Device.bindBufferMemory(stagingBuffer, stagingBufferMemory, 0);

        void* data;
        VK_CHECK(m_Device.mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data));
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        m_Device.unmapMemory(stagingBufferMemory);

        //Index buffer set up

        vk::BufferCreateInfo indexBufferInfo;
        indexBufferInfo.size = bufferSize;
        indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        indexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_Device.createBuffer(&indexBufferInfo, nullptr, &m_IndexBuffer));

        vk::MemoryRequirements indexMemRequirements;
        m_Device.getBufferMemoryRequirements(m_IndexBuffer, &indexMemRequirements);

        vk::MemoryAllocateInfo indexAllocInfo{};
        indexAllocInfo.allocationSize = indexMemRequirements.size;
        indexAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(indexMemRequirements.memoryTypeBits, m_GPU.getMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal);

        VK_CHECK(m_Device.allocateMemory(&indexAllocInfo, nullptr, &m_IndexBufferMemory));

        m_Device.bindBufferMemory(m_IndexBuffer, m_IndexBufferMemory, 0);

        //Copy data between buffers
        vk::CommandBuffer commandBuffer = m_CommandPool.BeginOneTimeSubmitCommand();

        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = bufferSize;
        commandBuffer.copyBuffer(stagingBuffer, m_IndexBuffer, 1, &copyRegion);

        m_CommandPool.EndOneTimeSubmitCommand(commandBuffer);

        //Finally we can destroy the staging buffer
        m_Device.destroyBuffer(stagingBuffer);
        m_Device.freeMemory(stagingBufferMemory);
    }

    void Mesh::Draw(vk::CommandBuffer commandBuffer) const
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

    void Mesh::Bind(vk::CommandBuffer commandBuffer) const
    {
        vk::Buffer buffers[] = { m_VertexBuffer };
        vk::DeviceSize offsets[] = { 0 };
        commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);

        if (m_HasIndexBuffer) 
        {
            commandBuffer.bindIndexBuffer(m_IndexBuffer, 0, vk::IndexType::eUint32);
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