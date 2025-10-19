#include "pch.h"
#include "render/RenderContext.h"
#include "core/Logger.h"
#include "platform/Platform.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace
{
    const std::vector<const char*> k_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

namespace prm{

    RenderContext::RenderContext(Platform& platform)
        : m_Platform(platform)
    {

    }

    RenderContext::~RenderContext()
    {
#if defined(VKB_DEBUG)
        if (m_DebugInfo.debug_utils_messenger)
        {
            Instance.destroyDebugUtilsMessengerEXT(m_DebugInfo.debug_utils_messenger, nullptr);
        }
        if (m_DebugInfo.debug_report_callback)
        {
            Instance.destroyDebugReportCallbackEXT(m_DebugInfo.debug_report_callback, nullptr);
        }
#endif

        if (Surface)
        {
            Instance.destroySurfaceKHR(Surface, nullptr);
        }
        if (Device)
        {
            Device.destroy();
        }
        if (Instance)
        {
            Instance.destroy();
        }
    }

	void RenderContext::Init() {
        auto& window = m_Platform.GetWindow();
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

        Surface = window.CreateSurface(Instance);

        FindPhysicalDevice();
        LOGI("Selected GPU: {}", GPU.getProperties().deviceName);

        CreateLogicalDevice(k_DeviceExtensions);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(Device);
	}

    uint32_t RenderContext::FindMemoryTypeIndex(uint32_t allowedTypes, vk::PhysicalDeviceMemoryProperties gpuProperties,
        vk::MemoryPropertyFlagBits desiredProperties)
    {
        //Memory types on the gpu
        for (size_t i = 0; i < gpuProperties.memoryTypeCount; ++i)
        {
            if ((allowedTypes & (1 << i)) &&  //Index of memory type must match corresponding type in allowedTypes
                (gpuProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties) //Desired property bit flags are part of memory type's property flags
            {
                return static_cast<uint32_t>(i);
            }
        }

        throw std::runtime_error("Could not find memory type on device with desired properties");
    }

    void RenderContext::CreateInstance(const std::vector<const char*>& requiredInstanceExtensions)
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

        const std::vector<const char*> requested_validation_layers({ "VK_LAYER_KHRONOS_validation" });

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
        if (m_DebugInfo.debug_utils)
        {
            info.pNext = &m_DebugInfo.debug_utils_create_info;
        }
        else
        {
            info.pNext = &m_DebugInfo.debug_report_create_info;
        }
#endif

        VK_CHECK(vk::createInstance(&info, nullptr, &Instance));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(Instance);

#if defined(VKB_DEBUG)
        vk::Result result;
        if (m_DebugInfo.debug_utils)
        {
            result = Instance.createDebugUtilsMessengerEXT(&m_DebugInfo.debug_utils_create_info, nullptr, &m_DebugInfo.debug_utils_messenger);
            if (result != vk::Result::eSuccess)
            {
                throw VulkanException(result, "Could not create debug utils messenger");
            }
        }
        else
        {
            result = Instance.createDebugReportCallbackEXT(&m_DebugInfo.debug_report_create_info, nullptr, &m_DebugInfo.debug_report_callback);
            if (result != vk::Result::eSuccess)
            {
                throw VulkanException(result, "Could not create debug report callback");
            }
        }
#endif
    }

    void RenderContext::CheckInstanceExtensionsSupport(const std::vector<const char*>& required_extensions)
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

    void RenderContext::FindPhysicalDevice()
    {
        // Querying valid physical devices on the machine
        uint32_t physical_device_count{ 0 };
        VK_CHECK(Instance.enumeratePhysicalDevices(&physical_device_count, nullptr));

        if (physical_device_count < 1)
        {
            throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
        }

        std::vector<vk::PhysicalDevice> physical_devices;
        physical_devices.resize(physical_device_count);
        VK_CHECK(Instance.enumeratePhysicalDevices(&physical_device_count, physical_devices.data()));

        // Find a discrete GPU
        for (const auto& gpu : physical_devices)
        {
            if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                auto familyIndices = GetQueueFamilyIndices(gpu);
                if (familyIndices.IsValid())
                {
                    GPU = gpu;
                    QueueIndices = familyIndices;
                    return;
                }
            }
        }

        LOGW("Couldn't find a discrete physical device, picking default GPU");
        GPU = physical_devices.at(0);
    }

    void RenderContext::CreateLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions)
    {
        CheckDeviceExtensionsSupport(requiredDeviceExtensions);

        const std::set<int32_t> queueFamilyIndices{ QueueIndices.presentFamily, QueueIndices.graphicsFamily };
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

        vk::PhysicalDeviceFeatures features{};
        features.samplerAnisotropy = true;

        vk::DeviceCreateInfo deviceInfo{};
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledDeviceExtensions.size());
        deviceInfo.ppEnabledExtensionNames = m_EnabledDeviceExtensions.data();
        deviceInfo.pEnabledFeatures = &features;

        VK_CHECK(GPU.createDevice(&deviceInfo, nullptr, &Device));

        //Queues are created at the same time as the device, we need to get the handle
        //Given logical device, of given queue family, of given queue index, get the handle
        GraphicsQueue = Device.getQueue(QueueIndices.graphicsFamily, 0);
        PresentQueue = Device.getQueue(QueueIndices.presentFamily, 0);
    }

    void RenderContext::CheckDeviceExtensionsSupport(const std::vector<const char*>& required_extensions)
    {
        uint32_t device_extension_count;
        VK_CHECK(GPU.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, nullptr));

        std::vector<vk::ExtensionProperties> available_device_extensions(device_extension_count);
        VK_CHECK(GPU.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, available_device_extensions.data()));

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

    QueueFamilyIndices RenderContext::GetQueueFamilyIndices(const vk::PhysicalDevice& gpu) const
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

            if (Surface)
            {
                const vk::Result result = gpu.getSurfaceSupportKHR(i, Surface, &present_supported);
                VK_CHECK(result);
            }

            if (present_supported)
            {
                res.presentFamily = i;
            }

            if (res.IsValid())
            {
                break;
            }

            ++i;
        }

        return res;
    }

}