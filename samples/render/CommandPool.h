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

        void RecordCommnadBuffers(vk::RenderPass renderPass, vk::Extent2D extent);

        vk::CommandBuffer& RequestCommandBuffer(uint32_t index);

        vk::CommandPool GetGraphicsPool() const { return m_GraphicsPool; }

    private:
        vk::CommandPool m_GraphicsPool;
        std::vector<vk::CommandBuffer> m_CommandBuffers;
        vk::Device m_Device;
    };
}


