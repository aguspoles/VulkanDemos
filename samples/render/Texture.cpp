#include "pch.h"
#include "core/Error.h"
#include "render/Texture.h"
#include "render/Buffer.h"
#include "render/CommandPool.h"

namespace prm {

	Texture::Texture(RenderContext& renderContext, CommandPool& commandPool, void* data, const Extent& imageSize)
		: m_RenderContext(renderContext)
		, m_CommandPool(commandPool)
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

		m_TextureImage = renderContext.Device.createImage(imageInfo);

		vk::MemoryRequirements memRequirements;
		renderContext.Device.getImageMemoryRequirements(m_TextureImage, &memRequirements);

		vk::MemoryAllocateInfo allocInfo;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = RenderContext::FindMemoryTypeIndex(memRequirements.memoryTypeBits, renderContext.GPU.getMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal);

		VK_CHECK(renderContext.Device.allocateMemory(&allocInfo, nullptr, &m_TextureImageMemory));
		renderContext.Device.bindImageMemory(m_TextureImage, m_TextureImageMemory, 0);

		TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		CopyBufferToImage(buffer->GetDeviceBuffer(), imageSize);
		TransitionImageLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::ImageViewCreateInfo viewInfo({}, m_TextureImage, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
		m_ImageView = m_RenderContext.Device.createImageView(viewInfo);

		//Save properties somewhere
		vk::PhysicalDeviceProperties properties = m_RenderContext.GPU.getProperties();
		vk::SamplerCreateInfo samplerInfo({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0, 1,
			properties.limits.maxSamplerAnisotropy, false, vk::CompareOp::eAlways);
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerInfo.unnormalizedCoordinates = false;
		samplerInfo.compareEnable = false;
		samplerInfo.compareOp = vk::CompareOp::eAlways;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		m_ImageSampler = m_RenderContext.Device.createSampler(samplerInfo);
	}

	Texture::~Texture()
	{
		m_RenderContext.Device.destroySampler(m_ImageSampler);
		m_RenderContext.Device.destroyImageView(m_ImageView);
		m_RenderContext.Device.destroyImage(m_TextureImage);
		m_RenderContext.Device.freeMemory(m_TextureImageMemory);
	}

	void Texture::TransitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
	{
		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.image = m_TextureImage;
		barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		auto commandBuffer = m_CommandPool.BeginOneTimeSubmitCommand();

		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);

		m_CommandPool.EndOneTimeSubmitCommand(commandBuffer);
	}

	void Texture::CopyBufferToImage(vk::Buffer buffer, const Extent& imageSize)
	{
		auto commandBuffer = m_CommandPool.BeginOneTimeSubmitCommand();

		vk::BufferImageCopy region(0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 }, { imageSize.width, imageSize.height, 1 });
		commandBuffer.copyBufferToImage(buffer, m_TextureImage, vk::ImageLayout::eTransferDstOptimal, { region });

		m_CommandPool.EndOneTimeSubmitCommand(commandBuffer);
	}

}
