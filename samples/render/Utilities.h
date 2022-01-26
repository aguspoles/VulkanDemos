#pragma once

namespace prm
{
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
}


