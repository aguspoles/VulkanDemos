#pragma once

namespace prm
{
    class CommandBuffer;

    class CommandPool
    {
    public:
        CommandPool(vk::Device& device, uint32_t queueIndex);

        CommandPool(const CommandPool&) = delete;

        CommandPool(CommandPool&& other) = delete;

        ~CommandPool();

        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool& operator=(CommandPool&&) = delete;

        CommandBuffer& RequestCommandBuffer(uint32_t index);

        vk::CommandPool GetHandle() const { return m_Handle; }

        vk::Device GetDevice() const { return m_Device; }

    private:
        vk::CommandPool m_Handle;
        std::vector<std::unique_ptr<CommandBuffer>> m_CommandBuffers;
        vk::Device m_Device;
    };
}


