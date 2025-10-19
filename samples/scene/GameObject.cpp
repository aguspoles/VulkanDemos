#include "pch.h"
#include "scene/GameObject.h"
#include "scene/Camera.h"

namespace prm {
    void GameObject::Render(vk::CommandBuffer commandBuffer, const Camera& camera, vk::PipelineLayout pipelineLayout) const
    {
        auto modelMatrix = transform.mat4();

        SimplePushConstantData push{};
        push.modelMatrix = modelMatrix;

        commandBuffer.pushConstants(
            pipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(SimplePushConstantData),
            &push);

        model->BindToRenderCommandBuffer(commandBuffer);
        model->DrawToRenderCommandBuffer(commandBuffer);
    }
}

