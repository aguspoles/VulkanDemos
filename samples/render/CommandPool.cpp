#include "pch.h"
#include "render/Utilities.h"
#include "render/CommandPool.h"
#include "render/CommandBuffer.h"
#include "core/Error.h"

namespace prm
{
    CommandPool::CommandPool(RenderContext& context)
        : m_RenderContext(context)
    {
        vk::CommandPoolCreateInfo info;
        info.queueFamilyIndex = m_RenderContext.r_QueueFamilyIndices.graphicsFamily;
        info.flags = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer };

        VK_CHECK(m_RenderContext.r_Device.createCommandPool(&info, nullptr, &m_Handle));
    }

    CommandPool::~CommandPool()
    {
        m_CommandBuffers.clear();
        m_RenderContext.r_Device.destroyCommandPool(m_Handle);
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

    vk::CommandBuffer CommandPool::BeginOneTimeSubmitCommand() const
    {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = m_Handle;
        allocInfo.commandBufferCount = 1;

        vk::CommandBuffer commandBuffer;
        VK_CHECK(m_RenderContext.r_Device.allocateCommandBuffers(&allocInfo, &commandBuffer));

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        VK_CHECK(commandBuffer.begin(&beginInfo));
        return commandBuffer;
    }

    void CommandPool::EndOneTimeSubmitCommand(vk::CommandBuffer command) const
    {
        command.end();

        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command;

        VK_CHECK(m_RenderContext.r_GraphicsQueue.submit(1, &submitInfo, nullptr));
        m_RenderContext.r_GraphicsQueue.waitIdle();

        m_RenderContext.r_Device.freeCommandBuffers(m_Handle, 1, &command);
    }
}
