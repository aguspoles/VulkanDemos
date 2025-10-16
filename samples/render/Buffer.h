#pragma once

namespace prm {
	struct RenderContext;
	class CommandBuffer;

	//Deafault vulkan buffer abstraction with staging buffer
	class Buffer {
	public:
		Buffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);
		Buffer(const Buffer&) = delete;
		Buffer(Buffer&&) = delete;

		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&&) = delete;

		virtual ~Buffer();

		template<class T, typename = std::enable_if<std::is_base_of<Buffer, T>::value || std::is_class<Buffer>::value>>
		static std::shared_ptr<T> CreateBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
		{
			auto buffer = std::make_shared<T>(renderContext, bufferSize, usage);
			buffer->Init();
			return buffer;
		}

		virtual void Init();
		virtual void UpdateDeviceData(const void* data, vk::CommandBuffer commandBuffer);

		vk::Buffer GetDeviceBuffer() const { return m_Buffer; }

	protected:
		void CreateBufferInDevice(const vk::MemoryPropertyFlagBits& memoryType);

	protected:
		RenderContext& m_RenderContext;

		vk::Buffer m_Buffer;
		vk::DeviceMemory m_DeviceMemory;
		vk::DeviceSize m_BufferSize;
		vk::BufferUsageFlags m_BufferUsage;

		vk::Buffer m_StagingBuffer;
		vk::DeviceMemory m_StagingBufferMemory;

		void* m_Data;
	};

	//Uniform buffer with constant mapped memory for per frame updates of its data
	class UniformBuffer : public Buffer {
	public:
		UniformBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);
		UniformBuffer(const UniformBuffer&) = delete;
		UniformBuffer(UniformBuffer&&) = delete;

		UniformBuffer& operator=(const UniformBuffer&) = delete;
		UniformBuffer& operator=(UniformBuffer&&) = delete;

		~UniformBuffer() override;

		void Init() override;
		void UpdateDeviceData(const void* data, vk::CommandBuffer commandBuffer) override;
	};
}