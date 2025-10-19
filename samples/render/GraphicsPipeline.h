#pragma once
#include "render/PipelineState.h"

namespace prm
{
    struct ShaderInfo
    {
        vk::ShaderStageFlagBits stage{ vk::ShaderStageFlagBits::eVertex };
        std::string entryPoint;
        std::vector<char> code;
    };

    class Pipeline
    {
    public:
        Pipeline(vk::Device& device);

        Pipeline(const Pipeline&) = delete;

        Pipeline(Pipeline&& other);

        virtual ~Pipeline();

        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline& operator=(Pipeline&&) = delete;

        vk::Pipeline GetHandle() const;

        const PipelineState& GetState() const;

    protected:
        vk::Device& m_Device;

        vk::Pipeline m_Handle;

        PipelineState m_State;
    };

    class GraphicsPipeline : public Pipeline
    {
    public:
        GraphicsPipeline(GraphicsPipeline&&) = default;

        virtual ~GraphicsPipeline() = default;

        GraphicsPipeline(vk::Device& device,
            vk::PipelineCache pipeline_cache,
            PipelineState& pipeline_state,
            const std::vector<ShaderInfo>& shaderInfos);
    };
}


