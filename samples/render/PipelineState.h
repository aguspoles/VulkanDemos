#pragma once
#include "core/Helpers.h"

namespace prm
{
    struct VertexInputState
    {
        std::vector<vk::VertexInputBindingDescription> bindings;

        std::vector<vk::VertexInputAttributeDescription> attributes;
    };

    struct InputAssemblyState
    {
        vk::PrimitiveTopology topology{ vk::PrimitiveTopology::eTriangleList };

        vk::Bool32 primitive_restart_enable{ VK_FALSE };
    };

    struct RasterizationState
    {
        vk::Bool32 depth_clamp_enable{ VK_FALSE };

        vk::Bool32 rasterizer_discard_enable{ VK_FALSE };

        vk::PolygonMode polygon_mode{ vk::PolygonMode::eFill };

        vk::CullModeFlags cull_mode{ vk::CullModeFlagBits::eBack };

        vk::FrontFace front_face{ vk::FrontFace::eClockwise };

        vk::Bool32 depth_bias_enable{ VK_FALSE };
    };

    struct ViewportState
    {
        uint32_t viewport_count{ 1 };

        uint32_t scissor_count{ 1 };
    };

    struct MultisampleState
    {
        vk::SampleCountFlagBits rasterization_samples{ vk::SampleCountFlagBits::e1 };

        vk::Bool32 sample_shading_enable{ VK_FALSE };

        float min_sample_shading{ 0.0f };

        vk::SampleMask sample_mask{ 0 };

        vk::Bool32 alpha_to_coverage_enable{ VK_FALSE };

        vk::Bool32 alpha_to_one_enable{ VK_FALSE };
    };

    struct StencilOpState
    {
        vk::StencilOp fail_op{ vk::StencilOp::eReplace };

        vk::StencilOp pass_op{ vk::StencilOp::eReplace };

        vk::StencilOp depth_fail_op{ vk::StencilOp::eReplace };

        vk::CompareOp compare_op{ vk::CompareOp::eNever };
    };

    struct DepthStencilState
    {
        vk::Bool32 depth_test_enable{ VK_TRUE };

        vk::Bool32 depth_write_enable{ VK_TRUE };

        // Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
        vk::CompareOp depth_compare_op{ vk::CompareOp::eGreater };

        vk::Bool32 depth_bounds_test_enable{ VK_FALSE };

        vk::Bool32 stencil_test_enable{ VK_FALSE };

        StencilOpState front{};

        StencilOpState back{};
    };

    struct ColorBlendAttachmentState
    {
        vk::Bool32 blend_enable{ VK_FALSE };

        vk::BlendFactor src_color_blend_factor{ vk::BlendFactor::eOne };

        vk::BlendFactor dst_color_blend_factor{ vk::BlendFactor::eZero };

        vk::BlendOp color_blend_op{ vk::BlendOp::eAdd };

        vk::BlendFactor src_alpha_blend_factor{ vk::BlendFactor::eOne };

        vk::BlendFactor dst_alpha_blend_factor{ vk::BlendFactor::eZero };

        vk::BlendOp alpha_blend_op{ vk::BlendOp::eAdd };

        vk::ColorComponentFlags color_write_mask{ vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
    };

    struct ColorBlendState
    {
        vk::Bool32 logic_op_enable{ VK_FALSE };

        vk::LogicOp logic_op{ vk::LogicOp::eClear };

        std::vector<ColorBlendAttachmentState> attachments;
    };

    class SpecializationConstantState
    {
    public:
        void Reset();

        bool IsDirty() const;

        void ClearDirty();

        template <class T>
        void SetConstant(uint32_t constant_id, const T& data);

        void SetConstant(uint32_t constant_id, const std::vector<uint8_t>& data);

        void SetSpecializationConstantState(const std::map<uint32_t, std::vector<uint8_t>>& state);

        const std::map<uint32_t, std::vector<uint8_t>>& GetSpecializationConstantState() const;

    private:
        bool dirty{ false };
        // Map tracking state of the Specialization Constants
        std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state;
    };

    template <class T>
    inline void SpecializationConstantState::SetConstant(std::uint32_t constant_id, const T& data)
    {
        SetConstant(constant_id, to_bytes(static_cast<std::uint32_t>(data)));
    }

    template <>
    inline void SpecializationConstantState::SetConstant<bool>(std::uint32_t constant_id, const bool& data)
    {
        SetConstant(constant_id, to_bytes(static_cast<std::uint32_t>(data)));
    }

    class PipelineState
    {
    public:
        void Reset();

        void SetSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data);

        void SetVertexInputState(const VertexInputState& vertex_input_state);

        void SetInputAssemblyState(const InputAssemblyState& input_assembly_state);

        void SetRasterizationState(const RasterizationState& rasterization_state);

        void SetViewportState(const ViewportState& viewport_state);

        void SetMultisampleState(const MultisampleState& multisample_state);

        void SetDepthStencilState(const DepthStencilState& depth_stencil_state);

        void SetColorBlendState(const ColorBlendState& color_blend_state);

        void SetSubpassIndex(uint32_t subpass_index);

        void SetPipelineLayout(const vk::PipelineLayout& layout);

        void SetRenderPass(const vk::RenderPass& pass);

        const SpecializationConstantState& GetSpecializationConstantState() const;

        const VertexInputState& GetVertexInputState() const;

        const InputAssemblyState& GetInputAssemblyState() const;

        const RasterizationState& GetRasterizationState() const;

        const ViewportState& GetViewportState() const;

        const MultisampleState& GetMultisampleState() const;

        const DepthStencilState& GetDepthStencilState() const;

        const ColorBlendState& GetColorBlendState() const;

        uint32_t GetSubpassIndex() const;

        vk::PipelineLayout GetPipelineLayout() const;

        vk::RenderPass GetRenderPass() const;

        bool IsDirty() const;

        void ClearDirty();

    private:
        bool m_Dirty{ false };

        SpecializationConstantState m_SpecializationConstantState{};

        VertexInputState m_VertexInputState{};

        InputAssemblyState m_InputAssemblyState{};

        RasterizationState m_RasterizationState{};

        ViewportState m_ViewportState{};

        MultisampleState m_MultisampleState{};

        DepthStencilState m_DepthStencilState{};

        ColorBlendState m_ColorBlendState{};

        uint32_t m_SubpassIndex{ 0U };

        vk::PipelineLayout m_Layout;
        vk::RenderPass m_RenderPass;
    };
}


