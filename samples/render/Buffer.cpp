#include "pch.h"
#include "Buffer.h"
#include "core/Error.h"
#include "Utilities.h"
#include "CommandPool.h"

namespace prm {
    std::shared_ptr<Buffer> Buffer::CreateBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
    {
        return std::make_shared<Buffer>(renderContext, bufferSize, usage);
    }

    Buffer::Buffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
        : m_RenderContext(renderContext)
        , m_BufferSize(bufferSize)
        , m_BufferUsage(usage)
    {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(renderContext.r_Device.createBuffer(&bufferInfo, nullptr, &m_Buffer));

        vk::MemoryRequirements memRequirements;
        renderContext.r_Device.getBufferMemoryRequirements(m_Buffer, &memRequirements);

        vk::MemoryAllocateInfo allocateInfo{};
        allocateInfo.allocationSize = memRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, renderContext.r_GPU.getMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal);

        VK_CHECK(m_RenderContext.r_Device.allocateMemory(&allocateInfo, nullptr, &m_DeviceMemory));

        renderContext.r_Device.bindBufferMemory(m_Buffer, m_DeviceMemory, 0);

        //Staging buffer set up
        vk::BufferCreateInfo stagingBufferInfo;
        stagingBufferInfo.size = m_BufferSize;
        stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_RenderContext.r_Device.createBuffer(&stagingBufferInfo, nullptr, &m_StagingBuffer));

        vk::MemoryRequirements stagingMemRequirements;
        m_RenderContext.r_Device.getBufferMemoryRequirements(m_StagingBuffer, &stagingMemRequirements);

        auto hostVisible = vk::MemoryPropertyFlagBits::eHostVisible;
        auto coherent = vk::MemoryPropertyFlagBits::eHostCoherent;
        auto mask = static_cast<vk::MemoryPropertyFlagBits> (
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(hostVisible) |
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(coherent)
            );

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = stagingMemRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeIndex(stagingMemRequirements.memoryTypeBits, m_RenderContext.r_GPU.getMemoryProperties(), mask);

        VK_CHECK(m_RenderContext.r_Device.allocateMemory(&allocInfo, nullptr, &m_StagingBufferMemory));

        m_RenderContext.r_Device.bindBufferMemory(m_StagingBuffer, m_StagingBufferMemory, 0);
    }

    Buffer::~Buffer()
    {
        m_RenderContext.r_Device.destroyBuffer(m_Buffer);
        m_RenderContext.r_Device.freeMemory(m_DeviceMemory);
    }

    void Buffer::UpdateDeviceData(const void* srcData, CommandPool& commandPool)
    {
        void* data;
        VK_CHECK(m_RenderContext.r_Device.mapMemory(m_StagingBufferMemory, 0, m_BufferSize, {}, &data));
        memcpy(data, srcData, static_cast<size_t>(m_BufferSize));
        m_RenderContext.r_Device.unmapMemory(m_StagingBufferMemory);

        //Copy data between buffers
        vk::CommandBuffer commandBuffer = commandPool.BeginOneTimeSubmitCommand();

        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = m_BufferSize;
        commandBuffer.copyBuffer(m_StagingBuffer, m_Buffer, 1, &copyRegion);

        commandPool.EndOneTimeSubmitCommand(commandBuffer);
    }
}
