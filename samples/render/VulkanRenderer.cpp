#include "pch.h"
#include "VulkanRenderer.h"

#include "CommandBuffer.h"
#include "CommandPool.h"
#include "core/Logger.h"
#include "render/GraphicsPipeline.h"
#include "render/Swapchain.h"
#include "render/CommandPool.h"
#include "render/Mesh.h"
#include "scene/GameObject.h"
#include "scene/Camera.h"

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

        for (auto layout : m_DescriptoSetLayouts)
            m_RenderContext.r_Device.destroyDescriptorSetLayout(layout);

        m_RenderContext.r_Device.destroyDescriptorPool(m_DescriptorPool);

        m_RenderContext.r_Device.destroyBuffer(m_MVPMatrixUniformBuffer);
        m_RenderContext.r_Device.freeMemory(m_MVPMatrixDeviceMemory);

        m_GraphicsCommandPool.reset();

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

        const auto vertexShaderCode = read_shader_file(m_VertexShaderPath);
        const auto fragmentShaderCode = read_shader_file(m_FragmentShaderPath);
        ShaderInfo vertInfo;
        vertInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertInfo.entryPoint = "main";
        vertInfo.code = vertexShaderCode;
        ShaderInfo fragInfo;
        fragInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragInfo.entryPoint = "main";
        fragInfo.code = fragmentShaderCode;

        m_ShaderInfos = { vertInfo, fragInfo };

        //Uniform buffer to store the mvp matrix
        CreateBuffer(m_MVPMatrixUniformBuffer, 16 * sizeof(float), m_MVPMatrixDeviceMemory, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer);

        CreateSwapchain();

        CreateDescriptorSets();

        CreatePipelineLayout();

        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(m_RenderContext.r_Device, m_PipeCache, m_PipelineState, m_ShaderInfos);

        m_GraphicsCommandPool = std::make_unique<CommandPool>(m_RenderContext);
    }

    void VulkanRenderer::Draw(const std::vector<GameObject>& gameObjects, const Camera& camera)
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

        Render(index, gameObjects, camera);

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

    void VulkanRenderer::CreateDescriptorSets()
    {
        vk::DescriptorSetLayoutBinding uniformBinding;
        uniformBinding.binding = 0;
        uniformBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uniformBinding.descriptorCount = 1;
        uniformBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        std::vector<vk::DescriptorSetLayoutBinding> bindings{ uniformBinding };

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCreateInfo.pBindings = &bindings[0];

        vk::DescriptorSetLayout descriptorSetLayout;
        descriptorSetLayout = m_RenderContext.r_Device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

        vk::DescriptorPoolSize descriptorPoolSize;
        descriptorPoolSize.type = vk::DescriptorType::eUniformBuffer;
        descriptorPoolSize.descriptorCount = 1;

        std::vector<vk::DescriptorPoolSize> descriptorPoolTypes{ descriptorPoolSize };

        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolTypes.size());
        descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolTypes[0];
        descriptorPoolCreateInfo.maxSets = 1;

        m_DescriptorPool = m_RenderContext.r_Device.createDescriptorPool(descriptorPoolCreateInfo);

        m_DescriptoSetLayouts.push_back(descriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_DescriptoSetLayouts.size());
        descriptorSetAllocateInfo.pSetLayouts = &m_DescriptoSetLayouts[0];

        m_DescriptorSets.resize(m_DescriptoSetLayouts.size());
        m_DescriptorSets = m_RenderContext.r_Device.allocateDescriptorSets(descriptorSetAllocateInfo);

        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = m_MVPMatrixUniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;
        std::vector<vk::DescriptorBufferInfo> bufferInfos{ bufferInfo };

        std::vector<BufferDescriptorInfo> bufferDescriptorInfos;
        BufferDescriptorInfo bufferDescriptor;
        bufferDescriptor.TargetDescriptorSet = m_DescriptorSets[0];
        bufferDescriptor.TargetDescriptorBinding = 0;
        bufferDescriptor.TargetArrayElement = 0;
        bufferDescriptor.TargetDescriptorType = vk::DescriptorType::eUniformBuffer;
        bufferDescriptor.BufferInfos = bufferInfos;
        bufferDescriptorInfos.push_back(bufferDescriptor);

        std::vector<vk::WriteDescriptorSet> writeDescriptors;
        for (auto& buffer_descriptor : bufferDescriptorInfos) {
            writeDescriptors.push_back({
              buffer_descriptor.TargetDescriptorSet,
              buffer_descriptor.TargetDescriptorBinding,
              buffer_descriptor.TargetArrayElement,
              static_cast<uint32_t>(buffer_descriptor.BufferInfos.size()),
              buffer_descriptor.TargetDescriptorType,
              nullptr,
              buffer_descriptor.BufferInfos.data(),
              nullptr
                });
        }

        m_RenderContext.r_Device.updateDescriptorSets(writeDescriptors, {});
    }

    void VulkanRenderer::CreatePipelineLayout()
    {
        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        vk::PipelineLayoutCreateInfo layoutInfo;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptoSetLayouts.size());
        layoutInfo.pSetLayouts = &m_DescriptoSetLayouts[0];
        if (m_PipeLayout)
        {
            m_RenderContext.r_Device.destroyPipelineLayout(m_PipeLayout);
        }
        VK_CHECK(m_RenderContext.r_Device.createPipelineLayout(&layoutInfo, nullptr, &m_PipeLayout));

        ColorBlendState blendState;
        ColorBlendAttachmentState blendAttState;
        blendState.attachments.push_back(blendAttState);

        VertexInputState vertexData;
        vertexData.attributes = Mesh::Vertex::getAttributeDescriptions();
        vertexData.bindings = Mesh::Vertex::getBindingDescriptions();

        m_PipelineState.SetPipelineLayout(m_PipeLayout);
        m_PipelineState.SetRenderPass(m_Swapchain->GetRenderPass());
        m_PipelineState.SetColorBlendState(blendState);
        m_PipelineState.SetVertexInputState(vertexData);
    }

    void VulkanRenderer::RecordCommandBuffer(uint32_t index, const std::vector<GameObject>& gameObjects, const Camera& camera) const
    {
        vk::CommandBufferBeginInfo info;
        info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse; //Means buffer can be resubmitted when it is already submitted and waiting for execution

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_Swapchain->GetRenderPass();   //Render pass to begin
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };  //Start point of render pass in pixels
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();  //Size of region to run render pass

        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.f, 0.01f, 0.2f, 1.0f} };
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        auto frame = m_Swapchain->GetFrameBuffer(index);
        renderPassInfo.framebuffer = frame;

        auto& buffer = m_GraphicsCommandPool->RequestCommandBuffer(index);
        auto bufferHandle = buffer.GetHandle();

        //Start recording command buffer
        VK_CHECK(bufferHandle.begin(&info));

        {
            bufferHandle.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

            {
                SetViewportAndScissor(bufferHandle);

                bufferHandle.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline->GetHandle());

                auto projectionView = camera.GetProjectionMatrix() * camera.GetViewMatrix();
                for (const auto& go : gameObjects)
                {
                    SimplePushConstantData push{};
                    auto modelMatrix = go.transform.mat4();
                    push.modelMatrix = modelMatrix;

                    bufferHandle.pushConstants(
                        m_PipeLayout,
                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                        0,
                        sizeof(SimplePushConstantData),
                        &push);

                    auto mvp = projectionView * modelMatrix;
                    SubmitDataToBuffer(16 * sizeof(float), m_MVPMatrixUniformBuffer, &mvp);

                    bufferHandle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipeLayout, 0, static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);

                    go.model->Bind(bufferHandle);
                    go.model->Draw(bufferHandle);
                }
            }

            bufferHandle.endRenderPass();
        }

        //End recording
        bufferHandle.end();
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

    void VulkanRenderer::Render(uint32_t index, const std::vector<GameObject>& gameObjects, const Camera& camera)
    {
        RecordCommandBuffer(index, gameObjects, camera);

        auto buffer = m_GraphicsCommandPool->RequestCommandBuffer(index).GetHandle();
        m_Swapchain->SubmitCommandBuffers(buffer, index);
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
        }

        //Pipeline depends on swapchain extent
        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(m_RenderContext.r_Device, m_PipeCache, m_PipelineState, m_ShaderInfos);
    }

    void VulkanRenderer::CreateBuffer(vk::Buffer& buffer, vk::DeviceSize bufferSize, vk::DeviceMemory& bufferMemory, vk::BufferUsageFlags usage)
    {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_RenderContext.r_Device.createBuffer(&bufferInfo, nullptr, &buffer));

        vk::MemoryRequirements memRequirements;
        m_RenderContext.r_Device.getBufferMemoryRequirements(buffer, &memRequirements);

        vk::MemoryAllocateInfo allocateInfo{};
        allocateInfo.allocationSize = memRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_RenderContext.r_GPU.getMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal);

        VK_CHECK(m_RenderContext.r_Device.allocateMemory(&allocateInfo, nullptr, &bufferMemory));

        m_RenderContext.r_Device.bindBufferMemory(buffer, bufferMemory, 0);
    }

    void VulkanRenderer::SubmitDataToBuffer(vk::DeviceSize bufferSize, vk::Buffer buffer, void* srcData) const
    {
        //Staging buffer set up
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        VK_CHECK(m_RenderContext.r_Device.createBuffer(&bufferInfo, nullptr, &stagingBuffer));

        vk::MemoryRequirements memRequirements;
        m_RenderContext.r_Device.getBufferMemoryRequirements(stagingBuffer, &memRequirements);

        auto hostVisible = vk::MemoryPropertyFlagBits::eHostVisible;
        auto coherent = vk::MemoryPropertyFlagBits::eHostCoherent;
        auto mask = static_cast<vk::MemoryPropertyFlagBits> (
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(hostVisible) |
            static_cast<std::underlying_type_t<vk::MemoryPropertyFlagBits>>(coherent)
            );

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_RenderContext.r_GPU.getMemoryProperties(), mask);

        VK_CHECK(m_RenderContext.r_Device.allocateMemory(&allocInfo, nullptr, &stagingBufferMemory));

        m_RenderContext.r_Device.bindBufferMemory(stagingBuffer, stagingBufferMemory, 0);

        void* data;
        VK_CHECK(m_RenderContext.r_Device.mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data));
        memcpy(data, srcData, static_cast<size_t>(bufferSize));
        m_RenderContext.r_Device.unmapMemory(stagingBufferMemory);

        //Copy data between buffers
        vk::CommandBuffer commandBuffer = m_GraphicsCommandPool->BeginOneTimeSubmitCommand();

        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = bufferSize;
        commandBuffer.copyBuffer(stagingBuffer, buffer, 1, &copyRegion);

        m_GraphicsCommandPool->EndOneTimeSubmitCommand(commandBuffer);
    }

    float VulkanRenderer::GetAspectRatio() const
    {
        return static_cast<float>(m_Swapchain->GetExtent().width) / static_cast<float>(m_Swapchain->GetExtent().height);
    }
}
