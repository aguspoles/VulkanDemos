#include "pch.h"
#include "core/Error.h"
#include "render/Texture.h"
#include "render/Buffer.h"
#include "render/Utilities.h"

namespace prm {

	Texture::Texture(RenderContext& renderContext, void* data, const Extent& imageSize)
		: m_RenderContext(renderContext)
	{
		assert(data);


		auto buffer = BufferBuilder::CreateBuffer<StagingBuffer>(renderContext, imageSize.BytesSize());
		buffer->UpdateData(data);

		vk::ImageCreateInfo imageInfo({}, 
			vk::ImageType::e2D, 
			vk::Format::eR8G8B8A8Srgb,
			{ imageSize.width, imageSize.height, 1 }, 
			1, 
			1, 
			vk::SampleCountFlagBits::e1, 
			vk::ImageTiling::eOptimal, //optimal for shader access 
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive, 0);


		m_TextureImage = renderContext.r_Device.createImage(imageInfo);

		vk::MemoryRequirements memRequirements;
		renderContext.r_Device.getImageMemoryRequirements(m_TextureImage, &memRequirements);

		vk::MemoryAllocateInfo allocInfo;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, renderContext.r_GPU.getMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal);

		VK_CHECK(renderContext.r_Device.allocateMemory(&allocInfo, nullptr, &m_TextureImageMemory));
		renderContext.r_Device.bindImageMemory(m_TextureImage, m_TextureImageMemory, 0);

	}

	Texture::~Texture()
	{
		m_RenderContext.r_Device.destroyImage(m_TextureImage);
		m_RenderContext.r_Device.freeMemory(m_TextureImageMemory);
	}

}
