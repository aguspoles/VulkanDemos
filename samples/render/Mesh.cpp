#include "pch.h"
#include "render/Mesh.h"

#include "core/Error.h"

namespace prm
{
    Mesh::Mesh(vk::PhysicalDevice gpu, vk::Device device, const std::vector<Vertex>& vertices)
        : m_GPU(gpu)
        , m_Device(device)
        , m_VertexCount(static_cast<uint32_t>(vertices.size()))
    {
        CreateVertexBuffer(vertices);
    }

    Mesh::~Mesh()
    {
        m_Device.destroyBuffer(m_VertexBuffer);
        m_Device.freeMemory(m_VertexBufferMemory);//Not using the memory anymore
    }

    void Mesh::CreateVertexBuffer(const std::vector<Vertex>& vertices)
    {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = static_cast<uint32_t>(sizeof(Vertex) * vertices.size());
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_Device.createBuffer(&bufferInfo, nullptr, &m_VertexBuffer));

        //Get buffer memory requirements
        vk::MemoryRequirements memRequirements = m_Device.getBufferMemoryRequirements(m_VertexBuffer);

        //Allocate memory for buffer
        auto hostVisible = vk::MemoryPropertyFlagBits::eHostVisible;
        auto coherent = vk::MemoryPropertyFlagBits::eHostCoherent;
        auto mask = static_cast<vk::MemoryPropertyFlagBits> (
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(hostVisible) |
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(coherent)
            );
        const uint32_t typeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, mask);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = typeIndex;

        //Allocate memory to device memory
        VK_CHECK(m_Device.allocateMemory(&allocInfo, nullptr, &m_VertexBufferMemory));

        //Allocate memory to given buffer
        m_Device.bindBufferMemory(m_VertexBuffer, m_VertexBufferMemory, 0);

        //Map local memory to device memory
        void* data; //Create a pointer to point in host memory(ram)
        VK_CHECK(m_Device.mapMemory(m_VertexBufferMemory, 0, bufferInfo.size, {}, &data));
        memcpy(data, vertices.data(), (size_t)bufferInfo.size); //Insert/copy the vertices to this chunk of memory

        //Because we set up the coherent type of memory, the data is sent/copy to the device memory, otherwise we would have to make a flush

        m_Device.unmapMemory(m_VertexBufferMemory); //Finally unmap the memory
    }

    uint32_t Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlagBits desiredProperties)
    {
        //Memory types on the gpu
        vk::PhysicalDeviceMemoryProperties memoryProperties = m_GPU.getMemoryProperties();

        for(size_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if((allowedTypes & (1 << i)) &&  //Index of memory type must match corresponding type in allowedTypes
                (memoryProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties) //Desired property bit flags are part of memory type's property flags
            {
                return static_cast<uint32_t>(i);
            }
        }

        throw std::runtime_error("Could not find memory type on device with desired properties");
    }
}
