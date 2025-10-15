#pragma once
#include "core/glm_defs.h"

namespace prm
{
    template <class T>
    uint32_t to_u32(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "T must be numeric");

        if (static_cast<uintmax_t>(value) > static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max()))
        {
            throw std::runtime_error("to_u32() failed, value is too big to be converted to uint32_t");
        }

        return static_cast<uint32_t>(value);
    }

    // from: https://stackoverflow.com/a/57595105
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };

    struct QueueFamilyIndices
    {
        int32_t graphicsFamily = -1;
        int32_t presentFamily = -1;

        bool IsValid() const { return graphicsFamily != -1 && presentFamily != -1; }
    };

    struct RenderContext
    {
        vk::Instance r_Instance;
        vk::Device r_Device;
        vk::PhysicalDevice r_GPU;
        vk::SurfaceKHR r_Surface;
        vk::Queue r_GraphicsQueue;
        vk::Queue r_PresentQueue;

        QueueFamilyIndices r_QueueFamilyIndices{};
    };


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

    struct SwapchainDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;  //surface properties
        std::vector<vk::SurfaceFormatKHR> formats; //surface image formats
        std::vector<vk::PresentModeKHR> presentModes; //How image should be presented on screen: FIFO, MAILBOX, etc
    };

    struct SwapchainImage
    {
        vk::Image image;
        vk::ImageView view;
    };

    struct ShaderInfo
    {
        vk::ShaderStageFlagBits stage{vk::ShaderStageFlagBits::eVertex};
        std::string entryPoint;
        std::vector<char> code;
    };

    struct SimplePushConstantData
    {
        glm::mat4 modelMatrix{ 1.0f };
        //alignas(16) glm::vec3 color{};
    };

    struct BufferDescriptorInfo 
    {
        vk::DescriptorSet TargetDescriptorSet;
        uint32_t TargetDescriptorBinding;
        uint32_t TargetArrayElement;
        vk::DescriptorType TargetDescriptorType;
        std::vector<vk::DescriptorBufferInfo> BufferInfos;
    };

    uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, vk::PhysicalDeviceMemoryProperties gpuProperties, vk::MemoryPropertyFlagBits desiredProperties);
}


