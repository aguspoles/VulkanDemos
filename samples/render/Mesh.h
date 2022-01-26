#pragma once
#include "render/Utilities.h"

namespace prm
{
    class Mesh
    {
    public:
        Mesh(vk::PhysicalDevice gpu, vk::Device device, const std::vector<Vertex>& vertices);
        ~Mesh();

        uint32_t GetVertexCount() const { return m_VertexCount; }

        vk::Buffer GetVertexBuffer() const { return m_VertexBuffer; }

    private:
        uint32_t m_VertexCount;
        vk::Device m_Device;
        vk::PhysicalDevice m_GPU;
        vk::Buffer m_VertexBuffer;
        vk::DeviceMemory m_VertexBufferMemory;

        void CreateVertexBuffer(const std::vector<Vertex>& vertices);

        uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlagBits desiredProperties);
    };
}


