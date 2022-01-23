#pragma once

namespace prm
{
    struct QueueFamilyIndices
    {
        int32_t graphicsFamily = -1;

        bool IsValid() const { return graphicsFamily != -1; }
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
}


