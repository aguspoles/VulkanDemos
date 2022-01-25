#include "pch.h"
#include "render/PipelineState.h"

bool operator==(const vk::VertexInputAttributeDescription& lhs, const vk::VertexInputAttributeDescription& rhs)
{
    return std::tie(lhs.binding, lhs.format, lhs.location, lhs.offset) == std::tie(rhs.binding, rhs.format, rhs.location, rhs.offset);
}

bool operator==(const vk::VertexInputBindingDescription& lhs, const vk::VertexInputBindingDescription& rhs)
{
    return std::tie(lhs.binding, lhs.inputRate, lhs.stride) == std::tie(rhs.binding, rhs.inputRate, rhs.stride);
}

bool operator==(const prm::ColorBlendAttachmentState& lhs, const prm::ColorBlendAttachmentState& rhs)
{
    return std::tie(lhs.alpha_blend_op, lhs.blend_enable, lhs.color_blend_op, lhs.color_write_mask, lhs.dst_alpha_blend_factor, lhs.dst_color_blend_factor, lhs.src_alpha_blend_factor, lhs.src_color_blend_factor) ==
        std::tie(rhs.alpha_blend_op, rhs.blend_enable, rhs.color_blend_op, rhs.color_write_mask, rhs.dst_alpha_blend_factor, rhs.dst_color_blend_factor, rhs.src_alpha_blend_factor, rhs.src_color_blend_factor);
}

bool operator!=(const prm::StencilOpState& lhs, const prm::StencilOpState& rhs)
{
    return std::tie(lhs.compare_op, lhs.depth_fail_op, lhs.fail_op, lhs.pass_op) != std::tie(rhs.compare_op, rhs.depth_fail_op, rhs.fail_op, rhs.pass_op);
}

bool operator!=(const prm::VertexInputState& lhs, const prm::VertexInputState& rhs)
{
    return lhs.attributes != rhs.attributes || lhs.bindings != rhs.bindings;
}

bool operator!=(const prm::InputAssemblyState& lhs, const prm::InputAssemblyState& rhs)
{
    return std::tie(lhs.primitive_restart_enable, lhs.topology) != std::tie(rhs.primitive_restart_enable, rhs.topology);
}

bool operator!=(const prm::RasterizationState& lhs, const prm::RasterizationState& rhs)
{
    return std::tie(lhs.cull_mode, lhs.depth_bias_enable, lhs.depth_clamp_enable, lhs.front_face, lhs.front_face, lhs.polygon_mode, lhs.rasterizer_discard_enable) !=
        std::tie(rhs.cull_mode, rhs.depth_bias_enable, rhs.depth_clamp_enable, rhs.front_face, rhs.front_face, rhs.polygon_mode, rhs.rasterizer_discard_enable);
}

bool operator!=(const prm::ViewportState& lhs, const prm::ViewportState& rhs)
{
    return lhs.viewport_count != rhs.viewport_count || lhs.scissor_count != rhs.scissor_count;
}

bool operator!=(const prm::MultisampleState& lhs, const prm::MultisampleState& rhs)
{
    return std::tie(lhs.alpha_to_coverage_enable, lhs.alpha_to_one_enable, lhs.min_sample_shading, lhs.rasterization_samples, lhs.sample_mask, lhs.sample_shading_enable) !=
        std::tie(rhs.alpha_to_coverage_enable, rhs.alpha_to_one_enable, rhs.min_sample_shading, rhs.rasterization_samples, rhs.sample_mask, rhs.sample_shading_enable);
}

bool operator!=(const prm::DepthStencilState& lhs, const prm::DepthStencilState& rhs)
{
    return std::tie(lhs.depth_bounds_test_enable, lhs.depth_compare_op, lhs.depth_test_enable, lhs.depth_write_enable, lhs.stencil_test_enable) !=
        std::tie(rhs.depth_bounds_test_enable, rhs.depth_compare_op, rhs.depth_test_enable, rhs.depth_write_enable, rhs.stencil_test_enable) ||
        lhs.back != rhs.back || lhs.front != rhs.front;
}

bool operator!=(const prm::ColorBlendState& lhs, const prm::ColorBlendState& rhs)
{
    return std::tie(lhs.logic_op, lhs.logic_op_enable) != std::tie(rhs.logic_op, rhs.logic_op_enable) ||
        lhs.attachments.size() != rhs.attachments.size() ||
        !std::equal(lhs.attachments.begin(), lhs.attachments.end(), rhs.attachments.begin(),
            [](const prm::ColorBlendAttachmentState& lhs, const prm::ColorBlendAttachmentState& rhs) {
                return lhs == rhs;
            });
}

namespace prm
{
    void SpecializationConstantState::Reset()
    {
        if (dirty)
        {
            specialization_constant_state.clear();
        }

        dirty = false;
    }

    bool SpecializationConstantState::IsDirty() const
    {
        return dirty;
    }

    void SpecializationConstantState::ClearDirty()
    {
        dirty = false;
    }

    void SpecializationConstantState::SetConstant(uint32_t constant_id, const std::vector<uint8_t>& value)
    {
        auto data = specialization_constant_state.find(constant_id);

        if (data != specialization_constant_state.end() && data->second == value)
        {
            return;
        }

        dirty = true;

        specialization_constant_state[constant_id] = value;
    }

    void SpecializationConstantState::SetSpecializationConstantState(const std::map<uint32_t, std::vector<uint8_t>>& state)
    {
        specialization_constant_state = state;
    }

    const std::map<uint32_t, std::vector<uint8_t>>& SpecializationConstantState::GetSpecializationConstantState() const
    {
        return specialization_constant_state;
    }

    void PipelineState::Reset()
    {
        ClearDirty();

        m_SpecializationConstantState.Reset();

        m_VertexInputState = {};

        m_InputAssemblyState = {};

        m_RasterizationState = {};

        m_MultisampleState = {};

        m_DepthStencilState = {};

        m_ColorBlendState = {};

        m_SubpassIndex = { 0U };
    }

    void PipelineState::SetSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data)
    {
        m_SpecializationConstantState.SetConstant(constant_id, data);

        if (m_SpecializationConstantState.IsDirty())
        {
            m_Dirty = true;
        }
    }

    void PipelineState::SetVertexInputState(const VertexInputState& new_vertex_input_state)
    {
        if (m_VertexInputState != new_vertex_input_state)
        {
            m_VertexInputState = new_vertex_input_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetInputAssemblyState(const InputAssemblyState& new_input_assembly_state)
    {
        if (m_InputAssemblyState != new_input_assembly_state)
        {
            m_InputAssemblyState = new_input_assembly_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetRasterizationState(const RasterizationState& new_rasterization_state)
    {
        if (m_RasterizationState != new_rasterization_state)
        {
            m_RasterizationState = new_rasterization_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetViewportState(const ViewportState& new_viewport_state)
    {
        if (m_ViewportState != new_viewport_state)
        {
            m_ViewportState = new_viewport_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetMultisampleState(const MultisampleState& new_multisample_state)
    {
        if (m_MultisampleState != new_multisample_state)
        {
            m_MultisampleState = new_multisample_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetDepthStencilState(const DepthStencilState& new_depth_stencil_state)
    {
        if (m_DepthStencilState != new_depth_stencil_state)
        {
            m_DepthStencilState = new_depth_stencil_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetColorBlendState(const ColorBlendState& new_color_blend_state)
    {
        if (m_ColorBlendState != new_color_blend_state)
        {
            m_ColorBlendState = new_color_blend_state;

            m_Dirty = true;
        }
    }

    void PipelineState::SetSubpassIndex(uint32_t new_subpass_index)
    {
        if (m_SubpassIndex != new_subpass_index)
        {
            m_SubpassIndex = new_subpass_index;

            m_Dirty = true;
        }
    }

    void PipelineState::SetPipelineLayout(const vk::PipelineLayout& layout)
    {
        if (m_Layout != layout)
        {
            m_Layout = layout;
            m_Dirty = true;
        }
    }

    void PipelineState::SetRenderPass(const vk::RenderPass& pass)
    {
        if (m_RenderPass != pass)
        {
            m_RenderPass = pass;
            m_Dirty = true;
        }
    }

    const SpecializationConstantState& PipelineState::GetSpecializationConstantState() const
    {
        return m_SpecializationConstantState;
    }

    const VertexInputState& PipelineState::GetVertexInputState() const
    {
        return m_VertexInputState;
    }

    const InputAssemblyState& PipelineState::GetInputAssemblyState() const
    {
        return m_InputAssemblyState;
    }

    const RasterizationState& PipelineState::GetRasterizationState() const
    {
        return m_RasterizationState;
    }

    const ViewportState& PipelineState::GetViewportState() const
    {
        return m_ViewportState;
    }

    const MultisampleState& PipelineState::GetMultisampleState() const
    {
        return m_MultisampleState;
    }

    const DepthStencilState& PipelineState::GetDepthStencilState() const
    {
        return m_DepthStencilState;
    }

    const ColorBlendState& PipelineState::GetColorBlendState() const
    {
        return m_ColorBlendState;
    }

    uint32_t PipelineState::GetSubpassIndex() const
    {
        return m_SubpassIndex;
    }

    vk::PipelineLayout PipelineState::GetPipelineLayout() const
    {
        return m_Layout;
    }

    vk::RenderPass PipelineState::GetRenderPass() const
    {
        return m_RenderPass;
    }

    bool PipelineState::IsDirty() const
    {
        return m_Dirty || m_SpecializationConstantState.IsDirty();
    }

    void PipelineState::ClearDirty()
    {
        m_Dirty = false;
        m_SpecializationConstantState.ClearDirty();
    }
}
