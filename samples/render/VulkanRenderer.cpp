#include "pch.h"
#include "VulkanRenderer.h"

#include "core/Logger.h"
#include "render/CommandBuffer.h"
#include "render/CommandPool.h"
#include "render/RenderContext.h"
#include "render/Swapchain.h"
#include "render/CommandPool.h"
#include "render/Mesh.h"
#include "render/Buffer.h"
#include "render/RenderableObject.h"
#include "render/Texture.h"
#include "scene/Camera.h"

namespace {
    struct CameraTransformUniformData
    {
        glm::mat4 viewMatrix{ 1.0f };
        glm::mat4 proyectionMatrix{ 1.0f };
    };
}

namespace prm
{
    VulkanRenderer::VulkanRenderer(Platform& platform)
        : m_Platform(platform)
        , m_RenderContext(nullptr)
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
    }

    void VulkanRenderer::Init()
    {
        m_RenderContext = std::make_unique<RenderContext>(m_Platform);
        m_RenderContext->Init();

        m_GraphicsCommandPool = std::make_unique<CommandPool>(*m_RenderContext);
    }

    void VulkanRenderer::Finish()
    {
        m_GraphicsCommandPool.reset();
        m_RenderContext.reset();
    }

    void VulkanRenderer::PrepareResources()
    {
        const auto vertexShaderCode = read_shader_file(m_VertexShaderPath);
        const auto fragmentShaderCode = read_shader_file(m_FragmentShaderPath);
        ShaderInfo vertInfo;
        vertInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertInfo.entryPoint = "main";
        vertInfo.code = vertexShaderCode;
        ShaderInfo fragInfo;
        fragInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragInfo.entryPoint = "main";
        fragInfo.code = fragmentShaderCode;

        m_ShaderInfos = { vertInfo, fragInfo };

        CreateSwapchain();

        //Uniform buffers to store per frame uniform data
        CreateUniformBuffers();

        CreateDescriptorSets();

        CreatePipelineLayout();

        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(m_RenderContext->Device, m_PipeCache, m_PipelineState, m_ShaderInfos);
    }

    void VulkanRenderer::CleanupResources()
    {
        m_RenderContext->Device.waitIdle(); //Wait for all resources to finish being used

        m_GraphicsPipeline.reset();

        if (m_PipeLayout)
        {
            m_RenderContext->Device.destroyPipelineLayout(m_PipeLayout);
        }

        for (auto& layout : m_DescriptoSetLayouts)
        {
            m_RenderContext->Device.destroyDescriptorSetLayout(layout);
        }

        m_RenderContext->Device.destroyDescriptorPool(m_DescriptorPool);

        for (size_t i = 0; i < m_Swapchain->GetMaxFramesInFlight(); ++i)
        {
            m_PerFrameUniformBuffers[i].reset();
        }

        m_Textures.clear();

        m_Swapchain.reset();
    }

    void VulkanRenderer::Draw(const std::vector<IRenderableObject*>& renderableObjects, const Camera& camera)
    {
        uint32_t index;

        auto res = m_Swapchain->AcquireNextImage(index);

        // Handle outdated error in acquire.
        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();
            res = m_Swapchain->AcquireNextImage(index);
        }

        if (res != vk::Result::eSuccess)
        {
            m_RenderContext->GraphicsQueue.waitIdle();
            return;
        }

        Render(index, renderableObjects, camera);

        // Handle Outdated error in present.
        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();
        }
        else if (res != vk::Result::eSuccess)
        {
            LOGE("Failed to present swapchain image.");
        }
    }

    void VulkanRenderer::CreateUniformBuffers()
    {
        for (size_t i = 0; i < m_Swapchain->GetMaxFramesInFlight(); ++i)
        {
            auto buffer = BufferBuilder::CreateBuffer<UniformBuffer>(*m_RenderContext, sizeof(CameraTransformUniformData), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer);
            m_PerFrameUniformBuffers.emplace_back(std::move(buffer));
        }
    }

    void VulkanRenderer::CreateSwapchain()
    {
        vk::SurfaceCapabilitiesKHR surface_properties = m_RenderContext->GPU.getSurfaceCapabilitiesKHR(m_RenderContext->Surface);
        const auto windowExtent = surface_properties.currentExtent;
        m_Swapchain = std::make_unique<Swapchain>(*m_RenderContext, windowExtent, std::move(m_Swapchain));
    }

    void VulkanRenderer::CreateDescriptorSets()
    {
        vk::DescriptorSetLayoutBinding uniformBinding;
        uniformBinding.binding = 0;
        uniformBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uniformBinding.descriptorCount = 1;
        uniformBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutBinding textureSamplerBinding;
        textureSamplerBinding.binding = 1;
        textureSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        textureSamplerBinding.descriptorCount = 1;
        textureSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

        std::vector<vk::DescriptorSetLayoutBinding> bindings{ uniformBinding, textureSamplerBinding };

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCreateInfo.pBindings = &bindings[0];

        vk::DescriptorSetLayout descriptorSetLayout;
        descriptorSetLayout = m_RenderContext->Device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

        //Only one layout, but one descriptor set for each frame in flight, all with the same layout. 
        // Unfortunately, we do need all the copies of the layout because the next function expects an array matching the number of sets
        m_DescriptoSetLayouts.emplace_back(descriptorSetLayout);
        std::vector<vk::DescriptorSetLayout> layouts(m_Swapchain->GetMaxFramesInFlight(), descriptorSetLayout);

        vk::DescriptorPoolSize descriptorPoolSizeUniform;
        descriptorPoolSizeUniform.type = vk::DescriptorType::eUniformBuffer;
        descriptorPoolSizeUniform.descriptorCount = m_Swapchain->GetMaxFramesInFlight();

        vk::DescriptorPoolSize descriptorPoolSizeSampler;
        descriptorPoolSizeSampler.type = vk::DescriptorType::eCombinedImageSampler;
        descriptorPoolSizeSampler.descriptorCount = m_Swapchain->GetMaxFramesInFlight();

        std::vector<vk::DescriptorPoolSize> descriptorPoolTypes{ descriptorPoolSizeUniform, descriptorPoolSizeSampler };

        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolTypes.size());
        descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolTypes[0];
        descriptorPoolCreateInfo.maxSets = m_Swapchain->GetMaxFramesInFlight();

        m_DescriptorPool = m_RenderContext->Device.createDescriptorPool(descriptorPoolCreateInfo);

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        descriptorSetAllocateInfo.pSetLayouts = &layouts[0];

        m_DescriptorSets.resize(layouts.size());
        m_DescriptorSets = m_RenderContext->Device.allocateDescriptorSets(descriptorSetAllocateInfo);

        std::vector<vk::WriteDescriptorSet> writeSets;
        for (size_t i = 0; i < m_Swapchain->GetMaxFramesInFlight(); ++i)
        {
            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = m_PerFrameUniformBuffers[i]->GetDeviceBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = VK_WHOLE_SIZE;

            vk::WriteDescriptorSet writeDescriptorSetUniform;
            writeDescriptorSetUniform.dstSet = m_DescriptorSets[i];
            writeDescriptorSetUniform.dstBinding = 0;
            writeDescriptorSetUniform.dstArrayElement = 0;
            writeDescriptorSetUniform.descriptorCount = 1;
            writeDescriptorSetUniform.descriptorType = vk::DescriptorType::eUniformBuffer;
            writeDescriptorSetUniform.pBufferInfo = &bufferInfo;

            writeSets.emplace_back(writeDescriptorSetUniform);

            for (auto& texture : m_Textures)
            {
                vk::DescriptorImageInfo imageInfo(texture->GetSampler(), texture->GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);

                vk::WriteDescriptorSet writeDescriptorSetSampler;
                writeDescriptorSetSampler.dstSet = m_DescriptorSets[i];
                writeDescriptorSetSampler.dstBinding = 1;
                writeDescriptorSetSampler.dstArrayElement = 0;
                writeDescriptorSetSampler.descriptorCount = 1;
                writeDescriptorSetSampler.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                writeDescriptorSetSampler.pImageInfo = &imageInfo;

                writeSets.emplace_back(writeDescriptorSetSampler);
            }
        }

        m_RenderContext->Device.updateDescriptorSets(writeSets, {});

    }

    void VulkanRenderer::CreatePipelineLayout()
    {
        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        vk::PipelineLayoutCreateInfo layoutInfo;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptoSetLayouts.size());
        layoutInfo.pSetLayouts = &m_DescriptoSetLayouts[0];
        if (m_PipeLayout)
        {
            m_RenderContext->Device.destroyPipelineLayout(m_PipeLayout);
        }
        VK_CHECK(m_RenderContext->Device.createPipelineLayout(&layoutInfo, nullptr, &m_PipeLayout));

        ColorBlendState blendState;
        ColorBlendAttachmentState blendAttState;
        blendState.attachments.push_back(blendAttState);

        VertexInputState vertexData;
        vertexData.attributes = Mesh::Vertex::getAttributeDescriptions();
        vertexData.bindings = Mesh::Vertex::getBindingDescriptions();

        m_PipelineState.SetPipelineLayout(m_PipeLayout);
        m_PipelineState.SetRenderPass(m_Swapchain->GetRenderPass());
        m_PipelineState.SetColorBlendState(blendState);
        m_PipelineState.SetVertexInputState(vertexData);
    }

    void VulkanRenderer::RecordCommandBuffer(uint32_t index, const std::vector<IRenderableObject*>& renderableObjects, const Camera& camera) const
    {
        vk::CommandBufferBeginInfo info;
        info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse; //Means buffer can be resubmitted when it is already submitted and waiting for execution

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_Swapchain->GetRenderPass();   //Render pass to begin
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };  //Start point of render pass in pixels
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();  //Size of region to run render pass

        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.f, 0.01f, 0.2f, 1.0f} };
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        auto frame = m_Swapchain->GetFrameBuffer(index);
        renderPassInfo.framebuffer = frame;

        auto& commandBuffer = m_GraphicsCommandPool->RequestCommandBuffer(index);
        auto commandBufferHandle = commandBuffer.GetHandle();

        //Start recording command buffer
        VK_CHECK(commandBufferHandle.begin(&info));

        {
            commandBufferHandle.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

            {
                SetViewportAndScissor(commandBufferHandle);

                commandBufferHandle.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline->GetHandle());
                commandBufferHandle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipeLayout, 0, 1, &m_DescriptorSets[index], 0, nullptr);

                for (const auto& object : renderableObjects)
                {
                    object->Render(commandBufferHandle, camera, m_PipeLayout);
                }

                CameraTransformUniformData uniformData{ camera.GetViewMatrix(), camera.GetProjectionMatrix() };

                m_PerFrameUniformBuffers[index]->UpdateData(&uniformData, commandBufferHandle);
            }

            commandBufferHandle.endRenderPass();
        }

        //End recording
        commandBufferHandle.end();
    }

    void VulkanRenderer::SetViewportAndScissor(vk::CommandBuffer buffer) const
    {
        vk::Viewport viewport{};
        viewport.width = static_cast<float>(m_Swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_Swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        buffer.setViewport(0, { viewport });

        vk::Rect2D scissor{};
        scissor.extent = m_Swapchain->GetExtent();
        buffer.setScissor(0, { scissor });
    }

    void VulkanRenderer::Render(uint32_t index, const std::vector<IRenderableObject*>& renderableObjects, const Camera& camera)
    {
        RecordCommandBuffer(index, renderableObjects, camera);

        auto buffer = m_GraphicsCommandPool->RequestCommandBuffer(index).GetHandle();
        m_Swapchain->SubmitCommandBuffers(buffer, index);
    }

    void VulkanRenderer::RecreateSwapchain()
    {
        vk::SurfaceCapabilitiesKHR surface_properties = m_RenderContext->GPU.getSurfaceCapabilitiesKHR(m_RenderContext->Surface);
        if (m_Swapchain)
        {
            // Only rebuild the swapchain if the dimensions have changed
            if (surface_properties.currentExtent.width == m_Swapchain->GetExtent().width &&
                surface_properties.currentExtent.height == m_Swapchain->GetExtent().height)
            {
                return;
            }
        }

        const auto windowExtent = surface_properties.currentExtent;

        m_RenderContext->Device.waitIdle(); //Wait for all resources to finish being used

        if (!m_Swapchain)
        {
            m_Swapchain = std::make_unique<Swapchain>(*m_RenderContext, windowExtent);
        }
        else
        {
            m_Swapchain = std::make_unique<Swapchain>(*m_RenderContext, windowExtent, std::move(m_Swapchain));
        }

        //Pipeline depends on swapchain extent
        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(m_RenderContext->Device, m_PipeCache, m_PipelineState, m_ShaderInfos);
    }

    void VulkanRenderer::AddTexture(const std::shared_ptr<Texture>& texture)
    {
        m_Textures.emplace_back(texture);
    }

    const RenderContext& VulkanRenderer::GetRenderContext() const
    {
        return *m_RenderContext;
    }

    RenderContext& VulkanRenderer::GetRenderContext()
    {
        return *m_RenderContext;
    }

    float VulkanRenderer::GetAspectRatio() const
    {
        return static_cast<float>(m_Swapchain->GetExtent().width) / static_cast<float>(m_Swapchain->GetExtent().height);
    }
}
