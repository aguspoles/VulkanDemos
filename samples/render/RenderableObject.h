#pragma once

namespace prm {
	class Camera;

	class IRenderableObject {
	public:
		virtual void Render(vk::CommandBuffer commandBuffer, const Camera& camera, vk::PipelineLayout pipelineLayout) const = 0;
	};
}
