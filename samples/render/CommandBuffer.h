#pragma once

namespace prm
{
    class CommandPool;

    class CommandBuffer
    {
    public:
        CommandBuffer(CommandPool& pool);

        CommandBuffer(const CommandBuffer&) = delete;

        CommandBuffer(CommandBuffer&& other) = delete;

        ~CommandBuffer();

        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer& operator=(CommandBuffer&&) = delete;

        vk::CommandBuffer GetHandle() const { return m_Handle; }

    private:
        CommandPool& m_CommandPool;
        vk::CommandBuffer m_Handle;
    };
}


