#include "pch.h"
#include "Buffer.h"
#include "core/Error.h"
#include "render/RenderContext.h"
#include "CommandBuffer.h"

namespace prm {

    Buffer::Buffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
        : m_RenderContext(renderContext)
        , m_BufferSize(bufferSize)
        , m_BufferUsage(usage)
        , m_Data(nullptr)
    {
       
    }

    Buffer::~Buffer()
    {
        m_RenderContext.Device.destroyBuffer(m_Buffer);
        m_RenderContext.Device.freeMemory(m_DeviceMemory);
    }

    void Buffer::CreateBufferInDevice(const vk::MemoryPropertyFlagBits& memoryType)
    {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = m_BufferSize;
        bufferInfo.usage = m_BufferUsage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_RenderContext.Device.createBuffer(&bufferInfo, nullptr, &m_Buffer));

        vk::MemoryRequirements memRequirements;
        m_RenderContext.Device.getBufferMemoryRequirements(m_Buffer, &memRequirements);

        vk::MemoryAllocateInfo allocateInfo{};
        allocateInfo.allocationSize = memRequirements.size;
        allocateInfo.memoryTypeIndex = RenderContext::FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_RenderContext.GPU.getMemoryProperties(), memoryType);

        VK_CHECK(m_RenderContext.Device.allocateMemory(&allocateInfo, nullptr, &m_DeviceMemory));

        m_RenderContext.Device.bindBufferMemory(m_Buffer, m_DeviceMemory, 0);
    }

    UniformBuffer::UniformBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
        : Buffer(renderContext, bufferSize, usage)
    {
    }

    UniformBuffer::~UniformBuffer()
    {
        m_RenderContext.Device.unmapMemory(m_DeviceMemory);
    }

    void UniformBuffer::Init()
    {
        CreateBufferInDevice(vk::MemoryPropertyFlagBits::eHostVisible);

        //No staging buffer involved, constant mapping of the memory and unmap on destroy
        VK_CHECK(m_RenderContext.Device.mapMemory(m_DeviceMemory, 0, m_BufferSize, {}, &m_Data));
    }

    void UniformBuffer::UpdateData(const void* srcData, vk::CommandBuffer commandBuffer)
    {
        memcpy(m_Data, srcData, static_cast<size_t>(m_BufferSize));
    }

    StagingBuffer::StagingBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize)
        : Buffer(renderContext, bufferSize, vk::BufferUsageFlagBits::eTransferSrc)
    {
    }

    StagingBuffer::~StagingBuffer()
    {
    }

    void StagingBuffer::Init()
    {
        vk::BufferCreateInfo stagingBufferInfo;
        stagingBufferInfo.size = m_BufferSize;
        stagingBufferInfo.usage = m_BufferUsage;
        stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_RenderContext.Device.createBuffer(&stagingBufferInfo, nullptr, &m_Buffer));

        vk::MemoryRequirements stagingMemRequirements;
        m_RenderContext.Device.getBufferMemoryRequirements(m_Buffer, &stagingMemRequirements);

        auto hostVisible = vk::MemoryPropertyFlagBits::eHostVisible;
        auto coherent = vk::MemoryPropertyFlagBits::eHostCoherent;
        auto mask = static_cast<vk::MemoryPropertyFlagBits> (
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(hostVisible) |
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(coherent)
            );

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = stagingMemRequirements.size;
        allocInfo.memoryTypeIndex = RenderContext::FindMemoryTypeIndex(stagingMemRequirements.memoryTypeBits, m_RenderContext.GPU.getMemoryProperties(), mask);

        VK_CHECK(m_RenderContext.Device.allocateMemory(&allocInfo, nullptr, &m_DeviceMemory));

        m_RenderContext.Device.bindBufferMemory(m_Buffer, m_DeviceMemory, 0);
    }

    void StagingBuffer::UpdateData(const void* srcData, vk::CommandBuffer commandBuffer)
    {
        VK_CHECK(m_RenderContext.Device.mapMemory(m_DeviceMemory, 0, m_BufferSize, {}, &m_Data));
        memcpy(m_Data, srcData, static_cast<size_t>(m_BufferSize));
        m_RenderContext.Device.unmapMemory(m_DeviceMemory);
    }

    MeshDataBuffer::MeshDataBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
        : Buffer(renderContext, bufferSize, usage)
    {
    }

    MeshDataBuffer::~MeshDataBuffer()
    {
        m_StagingBuffer.reset();
    }

    void MeshDataBuffer::Init()
    {
        CreateBufferInDevice(vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_StagingBuffer = BufferBuilder::CreateBuffer<StagingBuffer>(m_RenderContext, m_BufferSize);
    }

    void MeshDataBuffer::UpdateData(const void* srcData, vk::CommandBuffer commandBuffer)
    {
        m_StagingBuffer->UpdateData(srcData, commandBuffer);

        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = m_BufferSize;
        commandBuffer.copyBuffer(m_StagingBuffer->GetDeviceBuffer(), m_Buffer, 1, &copyRegion);
    }
}
