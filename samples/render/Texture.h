#pragma once

namespace prm {
	struct RenderContext;

	class Texture {

	public:
		struct Extent {
			uint32_t width;
			uint32_t height;

			uint32_t BytesSize() const { return width * height * 4; }
		};

		Texture(RenderContext& renderContext, void* data, const Extent& imageSize);
		~Texture();


	private:
		RenderContext& m_RenderContext;
		vk::Image m_TextureImage;
		vk::DeviceMemory m_TextureImageMemory;
	};
}