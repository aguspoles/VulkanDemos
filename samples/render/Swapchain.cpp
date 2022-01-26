#include "pch.h"
#include "render/Swapchain.h"
#include "core/Error.h"
#include "core/Logger.h"

#define MAX_FRAMES_IN_FLIGHT 2

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
    Swapchain::Swapchain(RenderContext& renderContext, vk::Extent2D windowExtent)
        : m_RenderContext(renderContext)
    {
        const SwapchainDetails swapchainDetails = GetSwapchainDetails(m_RenderContext.r_GPU);

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
        info.surface = m_RenderContext.r_Surface;
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
        if (m_RenderContext.r_QueueFamilyIndices.graphicsFamily != m_RenderContext.r_QueueFamilyIndices.presentFamily)
        {
            const uint32_t familyIndices[] = { static_cast<uint32_t>(m_RenderContext.r_QueueFamilyIndices.graphicsFamily),
                static_cast<uint32_t>(m_RenderContext.r_QueueFamilyIndices.presentFamily) };
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

        VK_CHECK(m_RenderContext.r_Device.createSwapchainKHR(&info, nullptr, &m_Handle));

        m_SwapchainImageFormat = selectedFormat.format;
        m_SwapchainExtent = extent;

        //Create the images
        uint32_t count;
        VK_CHECK(m_RenderContext.r_Device.getSwapchainImagesKHR(m_Handle, &count, nullptr));
        std::vector<vk::Image> images(count);
        VK_CHECK(m_RenderContext.r_Device.getSwapchainImagesKHR(m_Handle, &count, images.data()));

        for (const auto image : images)
        {
            SwapchainImage swapImage;
            swapImage.image = image;
            swapImage.view = CreateImageView(image, m_SwapchainImageFormat, vk::ImageAspectFlagBits::eColor);
            m_SwapchainImages.push_back(swapImage);
        }

        CreateSyncObjects();
    }

    Swapchain::~Swapchain()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            m_RenderContext.r_Device.destroySemaphore(imageAvailableSemaphores[i]);
            m_RenderContext.r_Device.destroySemaphore(renderFinishedSemaphores[i]);
            m_RenderContext.r_Device.destroyFence(inFlightFences[i]);
        }

        for (const auto& buffer : m_FrameBuffers)
        {
            m_RenderContext.r_Device.destroyFramebuffer(buffer);
        }
        for (const auto& image : m_SwapchainImages)
        {
            m_RenderContext.r_Device.destroyImageView(image.view);
        }
        if (m_Handle)
        {
            m_RenderContext.r_Device.destroySwapchainKHR(m_Handle);
        }
    }

    vk::Result Swapchain::AcquireNextImage(uint32_t& image)
    {
        VK_CHECK(m_RenderContext.r_Device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));

        vk::Result res;
        std::tie(res, image) = m_RenderContext.r_Device.acquireNextImageKHR(m_Handle, 
            UINT64_MAX, imageAvailableSemaphores[currentFrame]);

        return res;
    }

    vk::Result Swapchain::SubmitCommandBuffers(const vk::CommandBuffer* buffers, uint32_t imageIndex)
    {
        if (imagesInFlight[imageIndex])
        {
            VK_CHECK(m_RenderContext.r_Device.waitForFences(1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX));
        }
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        vk::SubmitInfo submitInfo;

        vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;

        vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VK_CHECK(m_RenderContext.r_Device.resetFences(1, &inFlightFences[currentFrame]));
        VK_CHECK(m_RenderContext.r_GraphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]));

        vk::PresentInfoKHR presentInfo;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = { m_Handle };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        auto result = m_RenderContext.r_PresentQueue.presentKHR(&presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
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

            VK_CHECK(m_RenderContext.r_Device.createFramebuffer(&info, nullptr, &m_FrameBuffers[i]));
        }
    }

    SwapchainDetails Swapchain::GetSwapchainDetails(const vk::PhysicalDevice& gpu) const
    {
        SwapchainDetails res;

        VK_CHECK(m_RenderContext.r_GPU.getSurfaceCapabilitiesKHR(m_RenderContext.r_Surface, &res.capabilities));

        uint32_t count = 0;
        VK_CHECK(m_RenderContext.r_GPU.getSurfaceFormatsKHR(m_RenderContext.r_Surface, &count, nullptr));
        res.formats.resize(count);
        VK_CHECK(m_RenderContext.r_GPU.getSurfaceFormatsKHR(m_RenderContext.r_Surface, &count, res.formats.data()));

        count = 0;
        VK_CHECK(m_RenderContext.r_GPU.getSurfacePresentModesKHR(m_RenderContext.r_Surface, &count, nullptr));
        res.presentModes.resize(count);
        VK_CHECK(m_RenderContext.r_GPU.getSurfacePresentModesKHR(m_RenderContext.r_Surface, &count, res.presentModes.data()));

        if (res.presentModes.empty() || res.formats.empty())
        {
            throw std::runtime_error("No formats or present modes are available for surface");
        }

        return res;
    }

    vk::SurfaceFormatKHR Swapchain::ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
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

    vk::PresentModeKHR Swapchain::ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>& available_present_modes)
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
        VK_CHECK(m_RenderContext.r_Device.createImageView(&info, nullptr, &view));

        return view;
    }

    void Swapchain::CreateSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(GetImagesCount(), VK_NULL_HANDLE);

        vk::SemaphoreCreateInfo semaphoreInfo;

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VK_CHECK(m_RenderContext.r_Device.createSemaphore(&semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
            VK_CHECK(m_RenderContext.r_Device.createSemaphore(&semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
            VK_CHECK(m_RenderContext.r_Device.createFence(&fenceInfo, nullptr, &inFlightFences[i]));
        }
    }

}
