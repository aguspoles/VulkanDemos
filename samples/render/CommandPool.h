#pragma once
#include "render/Utilities.h"

namespace prm
{
    class CommandPool
    {
    public:
        CommandPool(vk::Device& device, const QueueFamilyIndices& queueIndices);

        CommandPool(const CommandPool&) = delete;

        CommandPool(CommandPool&& other) = delete;

        ~CommandPool();

        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool& operator=(CommandPool&&) = delete;

        void CreateCommandBuffers(uint32_t count);

        vk::CommandBuffer& RequestCommandBuffer(uint32_t index);

        vk::CommandPool GetGraphicsPool() const { return m_GraphicsPool; }

        void FreeCommandBuffers();

        uint32_t GetBufferCount() const { return m_BufferCount; }

    private:
        vk::CommandPool m_GraphicsPool;
        std::vector<vk::CommandBuffer> m_CommandBuffers;
        vk::Device m_Device;
        uint32_t m_BufferCount = 0;
    };
}


