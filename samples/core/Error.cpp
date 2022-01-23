#include "pch.h"
#include "core/Error.h"

namespace prm
{
    VulkanException::VulkanException(const vk::Result result, const std::string& msg) :
        result{ result },
        std::runtime_error{ msg }
    {
        m_ErrorMessage = std::string(std::runtime_error::what()) + std::string{ " : " } + to_string(result);
    }

    const char* VulkanException::what() const noexcept
    {
        return m_ErrorMessage.c_str();
    }
}
