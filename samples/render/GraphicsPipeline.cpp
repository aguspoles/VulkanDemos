#include "pch.h"
#include "render/GraphicsPipeline.h"

#include "core/Error.h"

namespace prm
{
    Pipeline::Pipeline(vk::Device& device) :
        m_Device{ device }
    {}

    Pipeline::Pipeline(Pipeline&& other) :
        m_Device{ other.m_Device },
        m_Handle{ other.m_Handle },
        m_State{ other.m_State }
    {
        other.m_Handle = VK_NULL_HANDLE;
    }

    Pipeline::~Pipeline()
    {
        if (m_Handle)
        {
            m_Device.destroyPipeline(m_Handle, nullptr);
        }
    }

    vk::Pipeline Pipeline::GetHandle() const
    {
        return m_Handle;
    }

    const PipelineState& Pipeline::GetState() const
    {
        return m_State;
    }

    GraphicsPipeline::GraphicsPipeline(vk::Device& device,
        vk::PipelineCache pipeline_cache,
        PipelineState& pipeline_state,
        const std::vector<ShaderInfo>& shaderInfos) :
        Pipeline{ device }
    {
        std::vector<vk::PipelineShaderStageCreateInfo> stage_create_infos;
        std::vector<vk::ShaderModule> shaderModules;

        // Create specialization info from tracked state. This is shared by all shaders.
        std::vector<uint8_t> data{};
        std::vector<vk::SpecializationMapEntry> map_entries{};

        const auto specialization_constant_state = pipeline_state.GetSpecializationConstantState().GetSpecializationConstantState();

        for (const auto specialization_constant : specialization_constant_state)
        {
            map_entries.push_back({ specialization_constant.first, static_cast<uint32_t>(data.size()), specialization_constant.second.size() });
            data.insert(data.end(), specialization_constant.second.begin(), specialization_constant.second.end());
        }

        vk::SpecializationInfo specialization_info{};
        specialization_info.mapEntryCount = static_cast<uint32_t>(map_entries.size());
        specialization_info.pMapEntries = map_entries.data();
        specialization_info.dataSize = data.size();
        specialization_info.pData = data.data();

        for (const auto& shaderInfo : shaderInfos)
        {
            vk::PipelineShaderStageCreateInfo stage_create_info;

            stage_create_info.stage = shaderInfo.stage;
            stage_create_info.pName = shaderInfo.entryPoint.c_str();

            vk::ShaderModuleCreateInfo vk_create_info;

            vk_create_info.codeSize = shaderInfo.code.size();
            vk_create_info.pCode = reinterpret_cast<const uint32_t*>(shaderInfo.code.data());

            vk::Result result = device.createShaderModule(&vk_create_info, nullptr, &stage_create_info.module);

            if (result != vk::Result::eSuccess)
            {
                throw VulkanException{ result };
            }

            stage_create_info.pSpecializationInfo = &specialization_info;

            stage_create_infos.push_back(stage_create_info);
            shaderModules.push_back(stage_create_info.module);
        }

        vk::GraphicsPipelineCreateInfo create_info;

        create_info.stageCount = static_cast<uint32_t>(stage_create_infos.size());
        create_info.pStages = stage_create_infos.data();

        vk::PipelineVertexInputStateCreateInfo vertex_input_state;

        vertex_input_state.pVertexAttributeDescriptions = pipeline_state.GetVertexInputState().attributes.data();
        vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(pipeline_state.GetVertexInputState().attributes.size());

        vertex_input_state.pVertexBindingDescriptions = pipeline_state.GetVertexInputState().bindings.data();
        vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(pipeline_state.GetVertexInputState().bindings.size());

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;

        input_assembly_state.topology = pipeline_state.GetInputAssemblyState().topology;
        input_assembly_state.primitiveRestartEnable = pipeline_state.GetInputAssemblyState().primitive_restart_enable;

        vk::PipelineViewportStateCreateInfo viewport_state;

        viewport_state.viewportCount = pipeline_state.GetViewportState().viewport_count;
        viewport_state.scissorCount = pipeline_state.GetViewportState().scissor_count;

        vk::PipelineRasterizationStateCreateInfo rasterization_state;

        rasterization_state.depthClampEnable = pipeline_state.GetRasterizationState().depth_clamp_enable;
        rasterization_state.rasterizerDiscardEnable = pipeline_state.GetRasterizationState().rasterizer_discard_enable;
        rasterization_state.polygonMode = pipeline_state.GetRasterizationState().polygon_mode;
        rasterization_state.cullMode = pipeline_state.GetRasterizationState().cull_mode;
        rasterization_state.frontFace = pipeline_state.GetRasterizationState().front_face;
        rasterization_state.depthBiasEnable = pipeline_state.GetRasterizationState().depth_bias_enable;
        rasterization_state.depthBiasClamp = 1.0f;
        rasterization_state.depthBiasSlopeFactor = 1.0f;
        rasterization_state.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisample_state;

        multisample_state.sampleShadingEnable = pipeline_state.GetMultisampleState().sample_shading_enable;
        multisample_state.rasterizationSamples = pipeline_state.GetMultisampleState().rasterization_samples;
        multisample_state.minSampleShading = pipeline_state.GetMultisampleState().min_sample_shading;
        multisample_state.alphaToCoverageEnable = pipeline_state.GetMultisampleState().alpha_to_coverage_enable;
        multisample_state.alphaToOneEnable = pipeline_state.GetMultisampleState().alpha_to_one_enable;

        if (pipeline_state.GetMultisampleState().sample_mask)
        {
            multisample_state.pSampleMask = &pipeline_state.GetMultisampleState().sample_mask;
        }

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;

        depth_stencil_state.depthTestEnable = pipeline_state.GetDepthStencilState().depth_test_enable;
        depth_stencil_state.depthWriteEnable = pipeline_state.GetDepthStencilState().depth_write_enable;
        depth_stencil_state.depthCompareOp = pipeline_state.GetDepthStencilState().depth_compare_op;
        depth_stencil_state.depthBoundsTestEnable = pipeline_state.GetDepthStencilState().depth_bounds_test_enable;
        depth_stencil_state.stencilTestEnable = pipeline_state.GetDepthStencilState().stencil_test_enable;
        depth_stencil_state.front.failOp = pipeline_state.GetDepthStencilState().front.fail_op;
        depth_stencil_state.front.passOp = pipeline_state.GetDepthStencilState().front.pass_op;
        depth_stencil_state.front.depthFailOp = pipeline_state.GetDepthStencilState().front.depth_fail_op;
        depth_stencil_state.front.compareOp = pipeline_state.GetDepthStencilState().front.compare_op;
        depth_stencil_state.front.compareMask = ~0U;
        depth_stencil_state.front.writeMask = ~0U;
        depth_stencil_state.front.reference = ~0U;
        depth_stencil_state.back.failOp = pipeline_state.GetDepthStencilState().back.fail_op;
        depth_stencil_state.back.passOp = pipeline_state.GetDepthStencilState().back.pass_op;
        depth_stencil_state.back.depthFailOp = pipeline_state.GetDepthStencilState().back.depth_fail_op;
        depth_stencil_state.back.compareOp = pipeline_state.GetDepthStencilState().back.compare_op;
        depth_stencil_state.back.compareMask = ~0U;
        depth_stencil_state.back.writeMask = ~0U;
        depth_stencil_state.back.reference = ~0U;

        vk::PipelineColorBlendStateCreateInfo color_blend_state;

        color_blend_state.logicOpEnable = pipeline_state.GetColorBlendState().logic_op_enable;
        color_blend_state.logicOp = pipeline_state.GetColorBlendState().logic_op;
        color_blend_state.attachmentCount = static_cast<uint32_t>(pipeline_state.GetColorBlendState().attachments.size());
        color_blend_state.pAttachments = reinterpret_cast<const vk::PipelineColorBlendAttachmentState*>(pipeline_state.GetColorBlendState().attachments.data());
        color_blend_state.blendConstants[0] = 1.0f;
        color_blend_state.blendConstants[1] = 1.0f;
        color_blend_state.blendConstants[2] = 1.0f;
        color_blend_state.blendConstants[3] = 1.0f;

        std::array<vk::DynamicState, 9> dynamic_states{
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eLineWidth,
            vk::DynamicState::eDepthBias,
            vk::DynamicState::eBlendConstants,
            vk::DynamicState::eDepthBounds,
            vk::DynamicState::eStencilCompareMask,
            vk::DynamicState::eStencilWriteMask,
            vk::DynamicState::eStencilReference,
        };

        vk::PipelineDynamicStateCreateInfo dynamic_state;

        dynamic_state.pDynamicStates = dynamic_states.data();
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());

        create_info.pVertexInputState = &vertex_input_state;
        create_info.pInputAssemblyState = &input_assembly_state;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterization_state;
        create_info.pMultisampleState = &multisample_state;
        create_info.pDepthStencilState = &depth_stencil_state;
        create_info.pColorBlendState = &color_blend_state;
        create_info.pDynamicState = &dynamic_state;

        create_info.layout = pipeline_state.GetPipelineLayout();
        create_info.renderPass = pipeline_state.GetRenderPass();
        create_info.subpass = pipeline_state.GetSubpassIndex();

        auto result = device.createGraphicsPipeline(pipeline_cache, create_info, nullptr);
        m_Handle = result.value;

        if (result.result != vk::Result::eSuccess)
        {
            throw VulkanException{ result.result, "Cannot create GraphicsPipelines" };
        }

        for (auto shader_module : shaderModules)
        {
            device.destroyShaderModule(shader_module, nullptr);
        }

        m_State = pipeline_state;
    }
}
