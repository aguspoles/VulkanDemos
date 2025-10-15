#pragma once

namespace prm {
	struct RenderContext;
	class CommandPool;

	class Buffer {
	public:
		Buffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);
		Buffer(const Buffer&) = delete;
		Buffer(Buffer&&) = delete;

		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&&) = delete;

		virtual ~Buffer();

		static std::shared_ptr<Buffer> CreateBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);

		void UpdateDeviceData(const void* data, CommandPool& commandPool);

		vk::Buffer GetDeviceBuffer() const { return m_Buffer; }

	protected:
		RenderContext& m_RenderContext;
		vk::Buffer m_Buffer;
		vk::DeviceMemory m_DeviceMemory;
		vk::DeviceSize m_BufferSize;
		vk::BufferUsageFlags m_BufferUsage;

		vk::Buffer m_StagingBuffer;
		vk::DeviceMemory m_StagingBufferMemory;
	};
}