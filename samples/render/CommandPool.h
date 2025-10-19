#pragma once
#include "render/RenderContext.h"

namespace prm
{
    struct RenderContext;
    class CommandBuffer;

    class CommandPool
    {
    public:
        CommandPool(RenderContext& context);

        CommandPool(const CommandPool&) = delete;

        CommandPool(CommandPool&& other) = delete;

        ~CommandPool();

        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool& operator=(CommandPool&&) = delete;

        CommandBuffer& RequestCommandBuffer(uint32_t index);

        vk::CommandBuffer BeginOneTimeSubmitCommand() const;
        void EndOneTimeSubmitCommand(vk::CommandBuffer command) const;

        vk::CommandPool GetHandle() const { return m_Handle; }

        vk::Device GetDevice() const { return m_RenderContext.Device; }

    private:
        vk::CommandPool m_Handle;
        std::vector<std::unique_ptr<CommandBuffer>> m_CommandBuffers;
        RenderContext& m_RenderContext;
    };
}


