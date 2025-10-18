#pragma once

namespace prm {
	struct RenderContext;
	class CommandPool;

	class Texture {

	public:
		struct Extent {
			uint32_t width;
			uint32_t height;

			uint32_t BytesSize() const { return width * height * 4; }
		};

		Texture(RenderContext& renderContext, CommandPool& commandPool, void* data, const Extent& imageSize);
		~Texture();

		const vk::Sampler& GetSampler() const { return m_ImageSampler; }
		const vk::ImageView& GetImageView() const { return m_ImageView; }

	private:
		void TransitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

		void CopyBufferToImage(vk::Buffer buffer, const Extent& imageSize);


	private:
		RenderContext& m_RenderContext;
		CommandPool& m_CommandPool;
		vk::Image m_TextureImage;
		vk::DeviceMemory m_TextureImageMemory;
		vk::ImageView m_ImageView;
		vk::Sampler m_ImageSampler;
	};
}