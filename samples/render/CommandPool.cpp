#include "pch.h"
#include "render/CommandPool.h"
#include "core/Error.h"

namespace prm
{
    CommandPool::CommandPool(vk::Device& device, const QueueFamilyIndices& queueIndices)
        : m_Device(device)
    {
        vk::CommandPoolCreateInfo info;
        info.queueFamilyIndex = queueIndices.graphicsFamily;
        info.flags = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer };

        VK_CHECK(device.createCommandPool(&info, nullptr, &m_GraphicsPool));
    }

    CommandPool::~CommandPool()
    {
        m_Device.destroyCommandPool(m_GraphicsPool); //It will destroy command buffers too
    }

    void CommandPool::CreateCommandBuffers(uint32_t count)
    {
        m_CommandBuffers.resize(count);

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = m_GraphicsPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = count;

        VK_CHECK(m_Device.allocateCommandBuffers(&allocInfo, m_CommandBuffers.data()));
    }

    void CommandPool::RecordCommnadBuffers(vk::RenderPass renderPass, vk::Extent2D extent)
    {
        
    }

    vk::CommandBuffer& CommandPool::RequestCommandBuffer(uint32_t index)
    {
        return m_CommandBuffers[index];
    }
}
