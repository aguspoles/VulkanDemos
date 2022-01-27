#include "pch.h"
#include "render/CommandPool.h"
#include "render/CommandBuffer.h"
#include "core/Error.h"

namespace prm
{
    CommandPool::CommandPool(vk::Device& device, const uint32_t queueIndex)
        : m_Device(device)
    {
        vk::CommandPoolCreateInfo info;
        info.queueFamilyIndex = queueIndex;
        info.flags = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer };

        VK_CHECK(device.createCommandPool(&info, nullptr, &m_Handle));
    }

    CommandPool::~CommandPool()
    {
        m_CommandBuffers.clear();
        m_Device.destroyCommandPool(m_Handle);
    }

    CommandBuffer& CommandPool::RequestCommandBuffer(uint32_t index)
    {
        if (index < m_CommandBuffers.size())
        {
            return *m_CommandBuffers.at(index);
        }

        m_CommandBuffers.emplace_back(std::make_unique<CommandBuffer>(*this));

        return *m_CommandBuffers.back();
    }
}
