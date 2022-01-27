#include "pch.h"
#include "render/CommandBuffer.h"
#include "render/CommandPool.h"
#include "core/Error.h"

namespace prm
{
    CommandBuffer::CommandBuffer(CommandPool& pool)
        : m_CommandPool(pool)
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = m_CommandPool.GetHandle();
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        VK_CHECK(m_CommandPool.GetDevice().allocateCommandBuffers(&allocInfo, &m_Handle));
    }

    CommandBuffer::~CommandBuffer()
    {
        // Destroy command buffer
        if (m_Handle)
        {
            m_CommandPool.GetDevice().freeCommandBuffers(m_CommandPool.GetHandle(), 1, &m_Handle);
        }
    }
}
