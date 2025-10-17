#pragma once

namespace prm {
	struct RenderContext;
	class CommandBuffer;

	class BufferBuilder {
	public:
		template<class T, typename = std::enable_if<std::is_base_of<class Buffer, T>::value>>
		static std::shared_ptr<T> CreateBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage)
		{
			auto buffer = std::make_shared<T>(renderContext, bufferSize, usage);
			buffer->Init();
			return buffer;
		}

		template<class T, typename = std::enable_if<std::is_base_of<class Buffer, T>::value>>
		static std::shared_ptr<T> CreateBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize)
		{
			auto buffer = std::make_shared<T>(renderContext, bufferSize);
			buffer->Init();
			return buffer;
		}
	};

	//Deafault vulkan buffer abstraction
	class Buffer {
	public:
		Buffer(const Buffer&) = delete;
		Buffer(Buffer&&) = delete;

		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&&) = delete;

		virtual ~Buffer();

		virtual void Init() = 0;
		virtual void UpdateData(const void* data, vk::CommandBuffer commandBuffer) = 0;

		vk::Buffer GetDeviceBuffer() const { return m_Buffer; }

	protected:
		Buffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);

		void CreateBufferInDevice(const vk::MemoryPropertyFlagBits& memoryType);

	protected:
		RenderContext& m_RenderContext;

		vk::Buffer m_Buffer{};
		vk::DeviceMemory m_DeviceMemory{};
		vk::DeviceSize m_BufferSize;
		vk::BufferUsageFlags m_BufferUsage;

		void* m_Data;
	};

	//Use for vertex and index data
	class MeshDataBuffer : public Buffer {
	public:
		MeshDataBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);

		~MeshDataBuffer() override;

		void Init() override;
		void UpdateData(const void* data, vk::CommandBuffer commandBuffer) override;

	private:
		std::shared_ptr<class StagingBuffer> m_StagingBuffer;
	};

	//Uniform buffer with constant mapped memory for per frame updates of its data
	class UniformBuffer : public Buffer {
	public:
		UniformBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage);

		~UniformBuffer() override;

		void Init() override;
		void UpdateData(const void* data, vk::CommandBuffer commandBuffer) override;
	};

	class StagingBuffer : public Buffer {
	public:
		StagingBuffer(RenderContext& renderContext, vk::DeviceSize bufferSize);

		~StagingBuffer() override;

		void Init() override;
		void UpdateData(const void* data, vk::CommandBuffer commandBuffer = nullptr) override;
	};
}