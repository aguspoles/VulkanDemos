#pragma once
#include "core/Error.h"

namespace prm {
	class Platform;

	struct QueueFamilyIndices
	{
		int32_t graphicsFamily = -1;
		int32_t presentFamily = -1;

		bool IsValid() const { return graphicsFamily != -1 && presentFamily != -1; }
	};

	struct RenderContext
	{
		RenderContext(Platform& platform);
		~RenderContext();

		void Init();
		static uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, vk::PhysicalDeviceMemoryProperties gpuProperties, vk::MemoryPropertyFlagBits desiredProperties);

		vk::Instance Instance{};
		vk::Device Device{};
		vk::PhysicalDevice GPU{};
		vk::SurfaceKHR Surface{};
		vk::Queue GraphicsQueue{};
		vk::Queue PresentQueue{};

		QueueFamilyIndices QueueIndices{};

	private:
		void CreateInstance(const std::vector<const char*>& requiredInstanceExtensions);
		void CheckInstanceExtensionsSupport(const std::vector<const char*>& required_extensions);

		void FindPhysicalDevice();

		void CreateLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions);
		void CheckDeviceExtensionsSupport(const std::vector<const char*>& required_extensions);

		QueueFamilyIndices GetQueueFamilyIndices(const vk::PhysicalDevice& gpu) const;

	private:
		Platform& m_Platform;
		std::vector<const char*> m_EnabledInstanceExtensions;
		std::vector<const char*> m_EnabledDeviceExtensions;

#if defined(VKB_DEBUG)
		struct DebugInfo
		{
			vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info{};
			vk::DebugReportCallbackCreateInfoEXT debug_report_create_info{};
			bool debug_utils = false;

			/**
			 * @brief Debug utils messenger callback for VK_EXT_Debug_Utils
			 */
			vk::DebugUtilsMessengerEXT debug_utils_messenger;

			/**
			 * @brief The debug report callback
			 */
			vk::DebugReportCallbackEXT debug_report_callback;
		};

		DebugInfo m_DebugInfo;
#endif
	};
}
