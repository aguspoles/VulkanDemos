#include "pch.h"
#include "VulkanRenderer.h"
#include "core/Logger.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace
{
    const std::vector<const char*> k_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const std::vector<vk::SurfaceFormatKHR> k_SurfaceFormatPriorityList = {
        {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear} };

    std::string to_string(vk::SurfaceFormatKHR format)
    {
        return "myFormat"; //TODO 
    }

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data)
    {
        // Log debug messge
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
        }
        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
        }
        return VK_FALSE;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
        uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
        const char* layer_prefix, const char* message, void* /*user_data*/)
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            LOGE("{}: {}", layer_prefix, message);
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            LOGW("{}: {}", layer_prefix, message);
        }
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            LOGW("{}: {}", layer_prefix, message);
        }
        else
        {
            LOGI("{}: {}", layer_prefix, message);
        }
        return VK_FALSE;
    }
#endif

    bool validate_layers(const std::vector<const char*>& required,
        const std::vector<vk::LayerProperties>& available)
    {
        for (auto layer : required)
        {
            bool found = false;
            for (auto& available_layer : available)
            {
                if (strcmp(available_layer.layerName, layer) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                LOGE("Validation Layer {} not found", layer);
                return false;
            }
        }

        return true;
    }
}        // namespace

namespace prm
{
    VulkanRenderer::VulkanRenderer()
    {
        
    }

    VulkanRenderer::~VulkanRenderer()
    {
#if defined(VKB_DEBUG)
        if (m_DebugInfo.debug_utils_messenger)
        {
            m_Instance.destroyDebugUtilsMessengerEXT(m_DebugInfo.debug_utils_messenger, nullptr);
        }
        if (m_DebugInfo.debug_report_callback)
        {
            m_Instance.destroyDebugReportCallbackEXT(m_DebugInfo.debug_report_callback, nullptr);
        }
#endif

        for(auto image : m_SwapchainImages)
        {
            m_LogicalDevice.destroyImageView(image.view);
        }
        if(m_Swapchain)
        {
            m_LogicalDevice.destroySwapchainKHR(m_Swapchain);
        }
        if (m_Surface)
        {
            m_Instance.destroySurfaceKHR(m_Surface, nullptr);
        }
        if(m_LogicalDevice)
        {
            m_LogicalDevice.destroy();
        }
        if (m_Instance)
        {
            m_Instance.destroy();
        }
    }

    void VulkanRenderer::Init(Window& window)
    {
        static vk::DynamicLoader  dl;
        const auto vkGetInstanceProcAddr =
            dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        //Instance extensions
        std::vector<const char*> requiredInstanceExtensions;
        uint32_t extensionsCount = 0;
        const char** windowExtensions = window.GetInstanceExtensions(extensionsCount);

        for (uint32_t i = 0; i < extensionsCount; ++i)
        {
            requiredInstanceExtensions.emplace_back(windowExtensions[i]);
        }
        CreateInstance(requiredInstanceExtensions);

        m_Surface = window.CreateSurface(m_Instance);

        FindPhysicalDevice();
        LOGI("Selected GPU: {}", m_GPU.getProperties().deviceName);

        CreateLogicalDevice(k_DeviceExtensions);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_LogicalDevice);

        CreateSwapchain(window.GetExtent());
    }
     
    void VulkanRenderer::CreateInstance(const std::vector<const char*>& requiredInstanceExtensions)
    {
        vk::ApplicationInfo appInfo{};
        appInfo.apiVersion = VK_API_VERSION_1_1;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pApplicationName = "Demo";

        vk::InstanceCreateInfo info{};
        info.pApplicationInfo = &appInfo;

        CheckInstanceExtensionsSupport(requiredInstanceExtensions);
        info.enabledExtensionCount = static_cast<uint32_t>(m_EnabledInstanceExtensions.size());
        info.ppEnabledExtensionNames = m_EnabledInstanceExtensions.data();

#if defined(VKB_DEBUG)
        uint32_t instance_layer_count;
        VK_CHECK(vk::enumerateInstanceLayerProperties(&instance_layer_count, nullptr));

        std::vector<vk::LayerProperties> supported_validation_layers(instance_layer_count);
        VK_CHECK(vk::enumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers.data()));

        const std::vector<const char*> requested_validation_layers({"VK_LAYER_KHRONOS_validation"});

        if (validate_layers(requested_validation_layers, supported_validation_layers))
        {
            LOGI("Enabled Validation Layers:");
            for (const auto& layer : requested_validation_layers)
            {
                LOGI("    \t{}", layer);
            }
        }
        else
        {
            throw std::runtime_error("Required validation layers are missing.");
        }

        info.enabledLayerCount = static_cast<uint32_t>(requested_validation_layers.size());
        info.ppEnabledLayerNames = requested_validation_layers.data();
        if(m_DebugInfo.debug_utils)
        {
            info.pNext = &m_DebugInfo.debug_utils_create_info;
        }
        else
        {
            info.pNext = &m_DebugInfo.debug_report_create_info;
        }
#endif

        VK_CHECK(vk::createInstance(&info, nullptr, &m_Instance));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);

#if defined(VKB_DEBUG)
        vk::Result result;
        if (m_DebugInfo.debug_utils)
        {
            result = m_Instance.createDebugUtilsMessengerEXT(&m_DebugInfo.debug_utils_create_info, nullptr, &m_DebugInfo.debug_utils_messenger);
            if (result != vk::Result::eSuccess)
            {
                throw VulkanException(result, "Could not create debug utils messenger");
            }
        }
        else
        {
            result = m_Instance.createDebugReportCallbackEXT(&m_DebugInfo.debug_report_create_info, nullptr, &m_DebugInfo.debug_report_callback);
            if (result != vk::Result::eSuccess)
            {
                throw VulkanException(result, "Could not create debug report callback");
            }
        }
#endif
    }

    void VulkanRenderer::CheckInstanceExtensionsSupport(const std::vector<const char*>& required_extensions)
    {
        uint32_t instance_extension_count;
        VK_CHECK(vk::enumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

        std::vector<vk::ExtensionProperties> available_instance_extensions(instance_extension_count);
        VK_CHECK(vk::enumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

        auto extension_error = false;
        for (const auto* extension : required_extensions)
        {
            auto extension_name = extension;
            if (std::find_if(available_instance_extensions.begin(), available_instance_extensions.end(),
                [&extension_name](vk::ExtensionProperties available_extension) { return strcmp(available_extension.extensionName, extension_name) == 0; }) == available_instance_extensions.end())
            {
                    LOGE("Required instance extension {} not available, cannot run", extension_name);
                    extension_error = true;
            }
            m_EnabledInstanceExtensions.push_back(extension);
        }

#if defined(VKB_DEBUG)
        // Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
        for (auto& available_extension : available_instance_extensions)
        {
            if (strcmp(available_extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                m_DebugInfo.debug_utils = true;
                LOGI("{} is available, enabling it", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                m_EnabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }
        if (!m_DebugInfo.debug_utils)
        {
            m_EnabledInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        if (m_DebugInfo.debug_utils)
        {
            m_DebugInfo.debug_utils_create_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
            m_DebugInfo.debug_utils_create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
            m_DebugInfo.debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;
        }
        else
        {
            m_DebugInfo.debug_report_create_info.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning;
            m_DebugInfo.debug_report_create_info.pfnCallback = debug_callback;
        }
#endif

        if (extension_error)
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }
    }

    void VulkanRenderer::FindPhysicalDevice()
    {
        // Querying valid physical devices on the machine
        uint32_t physical_device_count{ 0 };
        VK_CHECK(m_Instance.enumeratePhysicalDevices(&physical_device_count, nullptr));

        if (physical_device_count < 1)
        {
            throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
        }

        std::vector<vk::PhysicalDevice> physical_devices;
        physical_devices.resize(physical_device_count);
        VK_CHECK(m_Instance.enumeratePhysicalDevices(&physical_device_count, physical_devices.data()));

        // Find a discrete GPU
        for (const auto& gpu : physical_devices)
        {
            if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                auto familyIndices = GetQueueFamilyIndices(gpu);
                if(familyIndices.IsValid())
                {
                    m_GPU = gpu;
                    m_QueueFamilyIndices = familyIndices;
                    return;
                }
            }
        }

        LOGW("Couldn't find a discrete physical device, picking default GPU");
        m_GPU = physical_devices.at(0);
    }

    QueueFamilyIndices VulkanRenderer::GetQueueFamilyIndices(const vk::PhysicalDevice& gpu) const
    {
        QueueFamilyIndices res{};

        const std::vector<vk::QueueFamilyProperties> queueFamilyProps = gpu.getQueueFamilyProperties();

        int32_t i = 0;
        for (const auto& queueFamily : queueFamilyProps)
        {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                res.graphicsFamily = i;
            }

            vk::Bool32 present_supported{ VK_FALSE };

            if (m_Surface)
            {
                const vk::Result result = gpu.getSurfaceSupportKHR(i, m_Surface, &present_supported);
                VK_CHECK(result);
            }

            if (present_supported)
            {
                res.presentFamily = i;
            }

            if(res.IsValid())
            {
                break;
            }

            ++i;
        }

        return res;
    }

    SwapchainDetails VulkanRenderer::GetSwapchainDetails(const vk::PhysicalDevice& gpu) const
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

        if(res.presentModes.empty() || res.formats.empty())
        {
            throw std::runtime_error("No formats or present modes are available for surface");
        }

        return res;
    }

    vk::SurfaceFormatKHR VulkanRenderer::ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
    {
        //Means all formats are defined
        if(formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
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

    vk::PresentModeKHR VulkanRenderer::ChoosePresentMode(vk::PresentModeKHR request_present_mode, const std::vector<vk::PresentModeKHR>& available_present_modes)
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

    vk::Extent2D VulkanRenderer::ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR capabilities, Window::Extent windowExtent)
    {
        const auto current_extent = capabilities.currentExtent;
        const auto min_image_extent = capabilities.minImageExtent;
        const auto max_image_extent = capabilities.maxImageExtent;

        if (current_extent.width == 0xFFFFFFFF)
        {
            return vk::Extent2D(windowExtent.width, windowExtent.height);
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

        return vk::Extent2D(windowExtent.width, windowExtent.height);
    }

    void VulkanRenderer::CreateLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions)
    {
        CheckDeviceExtensionsSupport(requiredDeviceExtensions);

        const std::set<int32_t> queueFamilyIndices{ m_QueueFamilyIndices.presentFamily, m_QueueFamilyIndices.graphicsFamily };
        std::vector<vk::DeviceQueueCreateInfo> queueInfos;

        for (const int32_t index : queueFamilyIndices)
        {
            vk::DeviceQueueCreateInfo queueInfo{};
            queueInfo.queueFamilyIndex = index;
            queueInfo.queueCount = 1;
            const float priority = 1.0f;
            queueInfo.pQueuePriorities = &priority;
            queueInfos.emplace_back(queueInfo);
        }

        vk::DeviceCreateInfo deviceInfo{};
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledDeviceExtensions.size());
        deviceInfo.ppEnabledExtensionNames = m_EnabledDeviceExtensions.data();
        const vk::PhysicalDeviceFeatures features{};
        deviceInfo.pEnabledFeatures = &features;

        VK_CHECK(m_GPU.createDevice(&deviceInfo, nullptr, &m_LogicalDevice));

        //Queues are created at the same time as the device, we need to get the handle
        //Given logical device, of given queue family, of given queue index, get the handle
        m_GraphicsQueue = m_LogicalDevice.getQueue(m_QueueFamilyIndices.graphicsFamily, 0);
        m_PresentationQueue = m_LogicalDevice.getQueue(m_QueueFamilyIndices.presentFamily, 0);
    }

    void VulkanRenderer::CheckDeviceExtensionsSupport(const std::vector<const char*>& required_extensions)
    {
        uint32_t device_extension_count;
        VK_CHECK(m_GPU.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, nullptr));

        std::vector<vk::ExtensionProperties> available_device_extensions(device_extension_count);
        VK_CHECK(m_GPU.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, available_device_extensions.data()));

        auto extension_error = false;
        for (const auto* extension : required_extensions)
        {
            auto extension_name = extension;
            if (std::find_if(available_device_extensions.begin(), available_device_extensions.end(),
                [&extension_name](vk::ExtensionProperties available_extension) { return strcmp(available_extension.extensionName, extension_name) == 0; }) == available_device_extensions.end())
            {
                LOGE("Required device extension {} not available, cannot run", extension_name);
                extension_error = true;
            }
            m_EnabledDeviceExtensions.push_back(extension);
        }

        if (extension_error)
        {
            throw std::runtime_error("Required device extensions are missing.");
        }
    }

    void VulkanRenderer::CreateSwapchain(Window::Extent windowExtent)
    {
        const SwapchainDetails swapchainDetails = GetSwapchainDetails(m_GPU);

        const auto selectedFormat = ChooseFormat(swapchainDetails.formats);
        const auto selectedPresentMode = ChoosePresentMode(vk::PresentModeKHR::eMailbox, swapchainDetails.presentModes);
        const auto extent = ChooseSwapchainExtent(swapchainDetails.capabilities, windowExtent);

        uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;
        if(swapchainDetails.capabilities.maxImageCount > 0 && 
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
        if(m_QueueFamilyIndices.graphicsFamily != m_QueueFamilyIndices.presentFamily)
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

        VK_CHECK(m_LogicalDevice.createSwapchainKHR(&info, nullptr, &m_Swapchain));

        m_SwapchainImageFormat = selectedFormat.format;
        m_SwapchainExtent = extent;

        //Create the images
        uint32_t count;
        VK_CHECK(m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain, &count, nullptr));
        std::vector<vk::Image> images(count);
        VK_CHECK(m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain, &count, images.data()));

        for(const auto image : images)
        {
            SwapchainImage swapImage;
            swapImage.image = image;
            swapImage.view = CreateImageView(image, m_SwapchainImageFormat, vk::ImageAspectFlagBits::eColor);
            m_SwapchainImages.push_back(swapImage);
        }
    }

    vk::ImageView VulkanRenderer::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect)
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
