#include "pch.h"
#include "VulkanRenderer.h"

#include "CommandPool.h"
#include "core/Logger.h"
#include "render/GraphicsPipeline.h"
#include "render/Swapchain.h"
#include "render/CommandPool.h"
#include "render/Mesh.h"

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

namespace prm
{
    VulkanRenderer::VulkanRenderer(Platform& platform)
        : m_Platform(platform)
    {
        
    }

    VulkanRenderer::~VulkanRenderer()
    {
#if defined(VKB_DEBUG)
        if (m_DebugInfo.debug_utils_messenger)
        {
            m_RenderContext.r_Instance.destroyDebugUtilsMessengerEXT(m_DebugInfo.debug_utils_messenger, nullptr);
        }
        if (m_DebugInfo.debug_report_callback)
        {
            m_RenderContext.r_Instance.destroyDebugReportCallbackEXT(m_DebugInfo.debug_report_callback, nullptr);
        }
#endif

        m_RenderContext.r_Device.waitIdle(); //Wait for all resources to finish being used

        m_CommandPool.reset();

        m_GraphicsPipeline.reset();
        if (m_PipeLayout)
        {
            m_RenderContext.r_Device.destroyPipelineLayout(m_PipeLayout);
        }

        m_Swapchain.reset();

        if (m_RenderContext.r_Surface)
        {
            m_RenderContext.r_Instance.destroySurfaceKHR(m_RenderContext.r_Surface, nullptr);
        }
        if (m_RenderContext.r_Device)
        {
            m_RenderContext.r_Device.destroy();
        }
        if (m_RenderContext.r_Instance)
        {
            m_RenderContext.r_Instance.destroy();
        }
    }

    void VulkanRenderer::Init()
    {
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

        m_RenderContext.r_Surface = window.CreateSurface(m_RenderContext.r_Instance);

        FindPhysicalDevice();
        LOGI("Selected GPU: {}", m_RenderContext.r_GPU.getProperties().deviceName);

        CreateLogicalDevice(k_DeviceExtensions);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_RenderContext.r_Device);

        //TODO move outside renderer
        const auto vertexShaderCode = read_shader_file("output/triangle_vert.spv");
        const auto fragmentShaderCode = read_shader_file("output/triangle_frag.spv");
        ShaderInfo vertInfo;
        vertInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertInfo.entryPoint = "main";
        vertInfo.code = vertexShaderCode;
        ShaderInfo fragInfo;
        fragInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragInfo.entryPoint = "main";
        fragInfo.code = fragmentShaderCode;

        m_ShaderInfos = { vertInfo, fragInfo };

        CreateSwapchain();

        CreatePipelineLayout();

        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(m_RenderContext.r_Device, m_PipeCache, m_PipelineState, m_ShaderInfos);

        m_CommandPool = std::make_unique<CommandPool>(m_RenderContext.r_Device, m_RenderContext.r_QueueFamilyIndices);
        m_CommandPool->CreateCommandBuffers(m_Swapchain->GetImagesCount());
    }

    void VulkanRenderer::Draw(const Mesh& mesh)
    {
        uint32_t index;

        auto res = m_Swapchain->AcquireNextImage(index);

        // Handle outdated error in acquire.
        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();
            res = m_Swapchain->AcquireNextImage(index);
        }

        if (res != vk::Result::eSuccess)
        {
            m_RenderContext.r_GraphicsQueue.waitIdle();
            return;
        }

        Render(index, mesh);

        // Handle Outdated error in present.
        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();
        }
        else if (res != vk::Result::eSuccess)
        {
            LOGE("Failed to present swapchain image.");
        }
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

        VK_CHECK(vk::createInstance(&info, nullptr, &m_RenderContext.r_Instance));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_RenderContext.r_Instance);

#if defined(VKB_DEBUG)
        vk::Result result;
        if (m_DebugInfo.debug_utils)
        {
            result = m_RenderContext.r_Instance.createDebugUtilsMessengerEXT(&m_DebugInfo.debug_utils_create_info, nullptr, &m_DebugInfo.debug_utils_messenger);
            if (result != vk::Result::eSuccess)
            {
                throw VulkanException(result, "Could not create debug utils messenger");
            }
        }
        else
        {
            result = m_RenderContext.r_Instance.createDebugReportCallbackEXT(&m_DebugInfo.debug_report_create_info, nullptr, &m_DebugInfo.debug_report_callback);
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
        VK_CHECK(m_RenderContext.r_Instance.enumeratePhysicalDevices(&physical_device_count, nullptr));

        if (physical_device_count < 1)
        {
            throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
        }

        std::vector<vk::PhysicalDevice> physical_devices;
        physical_devices.resize(physical_device_count);
        VK_CHECK(m_RenderContext.r_Instance.enumeratePhysicalDevices(&physical_device_count, physical_devices.data()));

        // Find a discrete GPU
        for (const auto& gpu : physical_devices)
        {
            if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                auto familyIndices = GetQueueFamilyIndices(gpu);
                if (familyIndices.IsValid())
                {
                    m_RenderContext.r_GPU = gpu;
                    m_RenderContext.r_QueueFamilyIndices = familyIndices;
                    return;
                }
            }
        }

        LOGW("Couldn't find a discrete physical device, picking default GPU");
        m_RenderContext.r_GPU = physical_devices.at(0);
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

            if (m_RenderContext.r_Surface)
            {
                const vk::Result result = gpu.getSurfaceSupportKHR(i, m_RenderContext.r_Surface, &present_supported);
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

    void VulkanRenderer::CreateSwapchain()
    {
        vk::SurfaceCapabilitiesKHR surface_properties = m_RenderContext.r_GPU.getSurfaceCapabilitiesKHR(m_RenderContext.r_Surface);
        const auto windowExtent = surface_properties.currentExtent;
        m_Swapchain = std::make_unique<Swapchain>(m_RenderContext, windowExtent, std::move(m_Swapchain));
    }

    void VulkanRenderer::CreatePipelineLayout()
    {
        vk::PipelineLayoutCreateInfo layoutInfo;
        layoutInfo.pSetLayouts = nullptr;
        layoutInfo.setLayoutCount = 0;
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;
        if (m_PipeLayout)
        {
            m_RenderContext.r_Device.destroyPipelineLayout(m_PipeLayout);
        }
        VK_CHECK(m_RenderContext.r_Device.createPipelineLayout(&layoutInfo, nullptr, &m_PipeLayout));

        ColorBlendState blendState;
        ColorBlendAttachmentState blendAttState;
        blendState.attachments.push_back(blendAttState);

        VertexInputState vertexData;
        vertexData.attributes.resize(1);
        vertexData.attributes[0].binding = 0;
        vertexData.attributes[0].location = 0;
        vertexData.attributes[0].offset = offsetof(Vertex, position); //Where this attribute is defined in a single vertex
        vertexData.attributes[0].format = vk::Format::eR32G32B32A32Sfloat;
        vertexData.bindings.resize(1);
        vertexData.bindings[0].binding = 0;
        vertexData.bindings[0].stride = sizeof(Vertex);
        vertexData.bindings[0].inputRate = vk::VertexInputRate::eVertex; //How to move between data after each vertex

        m_PipelineState.SetPipelineLayout(m_PipeLayout);
        m_PipelineState.SetRenderPass(m_Swapchain->GetRenderPass());
        m_PipelineState.SetColorBlendState(blendState);
        m_PipelineState.SetVertexInputState(vertexData);
    }

    void VulkanRenderer::RecordCommandBuffer(uint32_t index, const Mesh& mesh) const
    {
        vk::CommandBufferBeginInfo info;
        info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse; //Means buffer can be resubmitted when it is already submitted and waiting for execution

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_Swapchain->GetRenderPass();                     //Render pass to begin
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };      //Start point of render pass in pixels
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();  //Size of region to run render pass
        vk::ClearValue clearValue;
        clearValue.color = { std::array<float, 4>{0.f, 0.01f, 0.2f, 1.0f} };  //TODO depth attachment clear value
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        auto frame = m_Swapchain->GetFrameBuffer(index);
        renderPassInfo.framebuffer = frame;

        auto& buffer = m_CommandPool->RequestCommandBuffer(index);

        //Start recording command buffer
        VK_CHECK(buffer.begin(&info));

        buffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

        SetViewportAndScissor(buffer);

        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline->GetHandle());
        vk::Buffer vertexBuffer[] = { mesh.GetVertexBuffer() };
        vk::DeviceSize offset[] = { 0 };
        buffer.bindVertexBuffers(0, 1, vertexBuffer, offset);

        buffer.draw(mesh.GetVertexCount(), 1, 0, 0);

        buffer.endRenderPass();

        //End recording
        buffer.end();
    }

    void VulkanRenderer::SetViewportAndScissor(vk::CommandBuffer buffer) const
    {
        vk::Viewport viewport{};
        viewport.width = static_cast<float>(m_Swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_Swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        buffer.setViewport(0, { viewport });

        vk::Rect2D scissor{};
        scissor.extent = m_Swapchain->GetExtent();
        buffer.setScissor(0, { scissor });
    }

    void VulkanRenderer::Render(uint32_t index, const Mesh& mesh)
    {
        RecordCommandBuffer(index, mesh);

        m_Swapchain->SubmitCommandBuffers(&m_CommandPool->RequestCommandBuffer(index), index);
    }

    void VulkanRenderer::CreateLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions)
    {
        CheckDeviceExtensionsSupport(requiredDeviceExtensions);

        const std::set<int32_t> queueFamilyIndices{ m_RenderContext.r_QueueFamilyIndices.presentFamily, m_RenderContext.r_QueueFamilyIndices.graphicsFamily };
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

        VK_CHECK(m_RenderContext.r_GPU.createDevice(&deviceInfo, nullptr, &m_RenderContext.r_Device));

        //Queues are created at the same time as the device, we need to get the handle
        //Given logical device, of given queue family, of given queue index, get the handle
        m_RenderContext.r_GraphicsQueue = m_RenderContext.r_Device.getQueue(m_RenderContext.r_QueueFamilyIndices.graphicsFamily, 0);
        m_RenderContext.r_PresentQueue = m_RenderContext.r_Device.getQueue(m_RenderContext.r_QueueFamilyIndices.presentFamily, 0);
    }

    void VulkanRenderer::CheckDeviceExtensionsSupport(const std::vector<const char*>& required_extensions)
    {
        uint32_t device_extension_count;
        VK_CHECK(m_RenderContext.r_GPU.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, nullptr));

        std::vector<vk::ExtensionProperties> available_device_extensions(device_extension_count);
        VK_CHECK(m_RenderContext.r_GPU.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, available_device_extensions.data()));

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

    void VulkanRenderer::RecreateSwapchain()
    {
        vk::SurfaceCapabilitiesKHR surface_properties = m_RenderContext.r_GPU.getSurfaceCapabilitiesKHR(m_RenderContext.r_Surface);
        if (m_Swapchain)
        {
            // Only rebuild the swapchain if the dimensions have changed
            if (surface_properties.currentExtent.width == m_Swapchain->GetExtent().width &&
                surface_properties.currentExtent.height == m_Swapchain->GetExtent().height)
            {
                return;
            }
        }

        const auto windowExtent = surface_properties.currentExtent;

        m_RenderContext.r_Device.waitIdle(); //Wait for all resources to finish being used

        if (!m_Swapchain)
        {
            m_Swapchain = std::make_unique<Swapchain>(m_RenderContext, windowExtent);
        }
        else
        {
            m_Swapchain = std::make_unique<Swapchain>(m_RenderContext, windowExtent, std::move(m_Swapchain));
            if(m_Swapchain->GetImagesCount() != m_CommandPool->GetBufferCount())
            {
                m_CommandPool->FreeCommandBuffers();
                m_CommandPool->CreateCommandBuffers(m_Swapchain->GetImagesCount());
            }
        }

        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(m_RenderContext.r_Device, m_PipeCache, m_PipelineState, m_ShaderInfos);
    }
}
