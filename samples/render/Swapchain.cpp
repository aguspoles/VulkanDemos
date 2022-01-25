#include "pch.h"
#include "render/Swapchain.h"
#include "core/Error.h"
#include "core/Logger.h"

namespace 
{
    const std::vector<vk::SurfaceFormatKHR> k_SurfaceFormatPriorityList = {
        {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear} };

    std::string to_string(vk::SurfaceFormatKHR format)
    {
        return "myFormat"; //TODO 
    }
}

namespace prm
{
    Swapchain::Swapchain(vk::Device device, vk::PhysicalDevice gpu, vk::SurfaceKHR surface, const QueueFamilyIndices& queueFamilyIndices, vk::Extent2D windowExtent)
        : m_LogicalDevice(device),
        m_GPU(gpu),
        m_Surface(surface),
        m_QueueFamilyIndices(queueFamilyIndices)
    {
        const SwapchainDetails swapchainDetails = GetSwapchainDetails(m_GPU);

        const auto selectedFormat = ChooseFormat(swapchainDetails.formats);
        const auto selectedPresentMode = ChoosePresentMode(vk::PresentModeKHR::eMailbox, swapchainDetails.presentModes);
        const auto extent = ChooseSwapchainExtent(swapchainDetails.capabilities, windowExtent);

        uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;
        if (swapchainDetails.capabilities.maxImageCount > 0 &&
            swapchainDetails.capabilities.maxImageCount < imageCount)
        {
            imageCount = swapchainDetails.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR info;
        info.surface = m_Surface;
        info.imageFormat = selectedFormat.format;
        info.imageColorSpace = selectedFormat.colorSpace;
        info.presentMode = selectedPresentMode;
        info.imageExtent = extent;
        info.minImageCount = imageCount;
        info.imageArrayLayers = 1;
        info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; //What attachments images will be used as
        info.preTransform = swapchainDetails.capabilities.currentTransform; //Transform to perform on swapchain images
        info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; //Handle blending with external graphics e.g. other windows
        info.clipped = VK_TRUE;

        //If graphics and presentation families are different, then swapchain must let images be shared between families
        if (m_QueueFamilyIndices.graphicsFamily != m_QueueFamilyIndices.presentFamily)
        {
            const uint32_t familyIndices[] = { static_cast<uint32_t>(m_QueueFamilyIndices.graphicsFamily),
                static_cast<uint32_t>(m_QueueFamilyIndices.presentFamily) };
            info.imageSharingMode = vk::SharingMode::eConcurrent;
            info.queueFamilyIndexCount = 2;
            info.pQueueFamilyIndices = familyIndices;
        }
        else
        {
            info.imageSharingMode = vk::SharingMode::eExclusive;
            info.queueFamilyIndexCount = 0;
            info.pQueueFamilyIndices = nullptr;
        }

        VK_CHECK(m_LogicalDevice.createSwapchainKHR(&info, nullptr, &m_Handle));

        m_SwapchainImageFormat = selectedFormat.format;
        m_SwapchainExtent = extent;

        //Create the images
        uint32_t count;
        VK_CHECK(m_LogicalDevice.getSwapchainImagesKHR(m_Handle, &count, nullptr));
        std::vector<vk::Image> images(count);
        VK_CHECK(m_LogicalDevice.getSwapchainImagesKHR(m_Handle, &count, images.data()));

        for (const auto image : images)
        {
            SwapchainImage swapImage;
            swapImage.image = image;
            swapImage.view = CreateImageView(image, m_SwapchainImageFormat, vk::ImageAspectFlagBits::eColor);
            m_SwapchainImages.push_back(swapImage);
        }
    }

    Swapchain::~Swapchain()
    {
        for (const auto& semaphore : m_RecycledSemaphores)
        {
            m_LogicalDevice.destroySemaphore(semaphore.renderSemaphore);
            m_LogicalDevice.destroySemaphore(semaphore.presentSemaphore);
        }
        for (const auto& fence : m_Fences)
        {
            m_LogicalDevice.destroyFence(fence);
        }

        for (const auto& buffer : m_FrameBuffers)
        {
            m_LogicalDevice.destroyFramebuffer(buffer);
        }
        for (const auto& image : m_SwapchainImages)
        {
            m_LogicalDevice.destroyImageView(image.view);
        }
        if (m_Handle)
        {
            m_LogicalDevice.destroySwapchainKHR(m_Handle);
        }
    }

    vk::Result Swapchain::AcquireNextImage(uint32_t& image)
    {
        Semaphores acquireSemaphore;
        if (m_RecycledSemaphores.empty())
        {
            acquireSemaphore.renderSemaphore = m_LogicalDevice.createSemaphore({});
            acquireSemaphore.presentSemaphore = m_LogicalDevice.createSemaphore({});
        }
        else
        {
            acquireSemaphore = m_RecycledSemaphores.back();
            m_RecycledSemaphores.pop_back();
        }

        vk::Result res;
        std::tie(res, image) = m_LogicalDevice.acquireNextImageKHR(m_Handle, UINT64_MAX, acquireSemaphore.renderSemaphore);

        if (res != vk::Result::eSuccess)
        {
            m_RecycledSemaphores.push_back(acquireSemaphore);
            return res;
        }

        // If we have outstanding fences for this swapchain image, wait for them to complete first.
        // After begin frame returns, it is safe to reuse or delete resources which
        // were used previously.
        //
        // We wait for fences which completes N frames earlier, so we do not stall,
        // waiting for all GPU work to complete before this returns.
        // Normally, this doesn't really block at all,
        // since we're waiting for old frames to have been completed, but just in case.
        if (m_Fences[image])
        {
            VK_CHECK(m_LogicalDevice.waitForFences(m_Fences[image], true, UINT64_MAX));
            m_LogicalDevice.resetFences(m_Fences[image]);
        }

        //Recycle the old semaphore back into the semaphore manager.
        Semaphores oldSemaphore = m_SemaphoreByImageIndex[image];
        m_SemaphoreByImageIndex[image] = acquireSemaphore;
        if (oldSemaphore.renderSemaphore)
        {
            m_RecycledSemaphores.push_back(oldSemaphore);
        }

        return vk::Result::eSuccess;
    }

    void Swapchain::InitFrameBuffers(vk::RenderPass renderPass)
    {
        m_FrameBuffers.resize(m_SwapchainImages.size());

        for (size_t i = 0; i < m_FrameBuffers.size(); ++i)
        {
            std::array<vk::ImageView, 1> attachments{
                m_SwapchainImages[i].view
            };

            vk::FramebufferCreateInfo info;
            info.renderPass = renderPass;
            info.attachmentCount = static_cast<uint32_t>(attachments.size());
            info.pAttachments = attachments.data();  //One to one relations with the render pass attachments
            info.width = m_SwapchainExtent.width;
            info.height = m_SwapchainExtent.height;
            info.layers = 1;

            VK_CHECK(m_LogicalDevice.createFramebuffer(&info, nullptr, &m_FrameBuffers[i]));
        }

        m_SemaphoreByImageIndex.resize(m_SwapchainImages.size());
        m_Fences.resize(m_SwapchainImages.size());

        for(auto& fence : m_Fences)
        {
            fence = m_LogicalDevice.createFence({ vk::FenceCreateFlagBits::eSignaled });
        }
    }

    vk::Semaphore Swapchain::GetRenderSemaphore(uint32_t imageIndex) const
    {
        return m_SemaphoreByImageIndex[imageIndex].renderSemaphore;
    }

    vk::Semaphore Swapchain::GetPresentSemaphore(uint32_t imageIndex) const
    {
        return m_SemaphoreByImageIndex[imageIndex].presentSemaphore;
    }

    vk::Fence Swapchain::GetFence(uint32_t imageIndex) const
    {
        return m_Fences[imageIndex];
    }

    SwapchainDetails Swapchain::GetSwapchainDetails(const vk::PhysicalDevice & gpu) const
    {
        SwapchainDetails res;

        VK_CHECK(m_GPU.getSurfaceCapabilitiesKHR(m_Surface, &res.capabilities));

        uint32_t count = 0;
        VK_CHECK(m_GPU.getSurfaceFormatsKHR(m_Surface, &count, nullptr));
        res.formats.resize(count);
        VK_CHECK(m_GPU.getSurfaceFormatsKHR(m_Surface, &count, res.formats.data()));

        count = 0;
        VK_CHECK(m_GPU.getSurfacePresentModesKHR(m_Surface, &count, nullptr));
        res.presentModes.resize(count);
        VK_CHECK(m_GPU.getSurfacePresentModesKHR(m_Surface, &count, res.presentModes.data()));

        if (res.presentModes.empty() || res.formats.empty())
        {
            throw std::runtime_error("No formats or present modes are available for surface");
        }

        return res;
    }

    vk::SurfaceFormatKHR Swapchain::ChooseFormat(const std::vector<vk::SurfaceFormatKHR>&formats)
    {
        //Means all formats are defined
        if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
        {
            return k_SurfaceFormatPriorityList[0];
        }

        for (auto& surface_format : k_SurfaceFormatPriorityList)
        {
            auto surface_format_it = std::find_if(
                formats.begin(),
                formats.end(),
                [&surface_format](const vk::SurfaceFormatKHR& surface) {
                    if (surface.format == surface_format.format &&
                        surface.colorSpace == surface_format.colorSpace)
                    {
                        return true;
                    }

                    return false;
                });
            if (surface_format_it != formats.end())
            {
                LOGI("(Swapchain) Surface format ({}) selected.", ::to_string(*surface_format_it));
                return *surface_format_it;
            }
        }

        // If nothing found, default the first supporte surface format
        auto surface_format_it = formats.begin();
        LOGW("(Swapchain) Surface formats not supported. Selecting ({}).", ::to_string(*surface_format_it));

        return *surface_format_it;
    }

    vk::PresentModeKHR Swapchain::ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>&available_present_modes)
    {
        auto present_mode_it = std::find(available_present_modes.begin(), available_present_modes.end(), request_present_mode);

        if (present_mode_it == available_present_modes.end())
        {
            // If nothing found, always default to FIFO
            const vk::PresentModeKHR chosen_present_mode = vk::PresentModeKHR::eFifo;

            LOGW("(Swapchain) Present mode '{}' not supported. Selecting '{}'.", vk::to_string(request_present_mode), vk::to_string(chosen_present_mode));
            return chosen_present_mode;
        }
        else
        {
            LOGI("(Swapchain) Present mode selected: {}", vk::to_string(request_present_mode));
            return *present_mode_it;
        }
    }

    vk::Extent2D Swapchain::ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR capabilities, vk::Extent2D windowExtent)
    {
        const auto current_extent = capabilities.currentExtent;
        const auto min_image_extent = capabilities.minImageExtent;
        const auto max_image_extent = capabilities.maxImageExtent;

        if (current_extent.width == 0xFFFFFFFF)
        {
            return { windowExtent.width, windowExtent.height };
        }

        if (windowExtent.width < 1 || windowExtent.height < 1)
        {
            LOGW("(Swapchain) Image extent ({}, {}) not supported. Selecting ({}, {}).", windowExtent.width, windowExtent.height, current_extent.width, current_extent.height);
            return current_extent;
        }

        windowExtent.width = std::max(windowExtent.width, min_image_extent.width);
        windowExtent.width = std::min(windowExtent.width, max_image_extent.width);

        windowExtent.height = std::max(windowExtent.height, min_image_extent.height);
        windowExtent.height = std::min(windowExtent.height, max_image_extent.height);

        return { windowExtent.width, windowExtent.height };
    }

    vk::ImageView Swapchain::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect)
    {
        vk::ImageViewCreateInfo info;
        info.image = image;
        info.format = format;
        info.viewType = vk::ImageViewType::e2D;
        info.components.r = vk::ComponentSwizzle::eIdentity;
        info.components.g = vk::ComponentSwizzle::eIdentity;
        info.components.b = vk::ComponentSwizzle::eIdentity;
        info.components.a = vk::ComponentSwizzle::eIdentity;

        //Subresources allow the view to only view a part of an image
        info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        vk::ImageView view;
        VK_CHECK(m_LogicalDevice.createImageView(&info, nullptr, &view));

        return view;
    }

}
