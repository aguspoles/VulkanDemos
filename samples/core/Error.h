#pragma once
#include "core/Logger.h"

namespace prm
{
    class VulkanException : public std::runtime_error
    {
    public:
        VulkanException(vk::Result result, const std::string& msg = "Vulkan error");

        const char* what() const noexcept override;

        vk::Result result;

    private:
        std::string m_ErrorMessage;
    };
} 

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                 \
    do                                                              \
    {                                                               \
        vk::Result err = x;                                         \
        if (err != vk::Result::eSuccess)                            \
        {                                                           \
            LOGE("Detected Vulkan error: {}", to_string(err));      \
            abort();                                                \
        }                                                           \
    } while (0)

#define ASSERT_VK_HANDLE(handle)        \
    do                                  \
    {                                   \
        if ((handle) == VK_NULL_HANDLE) \
        {                               \
            LOGE("Handle is NULL");     \
            abort();                    \
        }                               \
    } while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#	define VKB_DEBUG
#endif
