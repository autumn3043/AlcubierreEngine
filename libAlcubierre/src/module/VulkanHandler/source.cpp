#include "module/VulkanHandler/public.h"
#include "module/VulkanHandler/private.h"

#include <utility>
#include <variant>

#define NULL_BIT 0x0

ModuleRegistryBundle VulkanHandler::bundle(
    [](void* registry) -> WrapperBaseClass* { return new VulkanHandler(registry); },
    {GRAPHICS_BACKEND},
    {WINDOW_SURFACE}, //Swapchain images point to window surface elements which must remain valid during shutdown to avoid a segmentation fault
    "VulkanHandler"
);

VulkanHandler::VulkanHandler(void* registry) 
    :   IGraphicsBackend_VulkanHandler(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        //We construct our Pimpl here because this constructor will not be invoked until registry requests this module.
        PrivatePtr = new VulkanHandlerIMPL(registry_ptr);
        //Inserting service pointers into WrapperBaseClass unordered map, which is where registry expects the services to be when we promise them in the bundle.
        Services = {{GRAPHICS_BACKEND, &IGraphicsBackend_VulkanHandler}};
    }

VulkanHandler::~VulkanHandler() {
    if(PrivatePtr) delete PrivatePtr;
}

void* VulkanHandler::GetBackendObjectImpl() {
    return PrivatePtr;
}

void VulkanHandler::drawFrameImpl() {
    return PrivatePtr->drawFrame();
}

VulkanHandlerIMPL::VulkanHandlerIMPL(Registry* registry) : registry_ptr(registry) {
    DebugManager::Punchcard bootstrapTimer;

    environment = new VulkanRuntimeEnvironmentComponent(this, registry_ptr);
    device = new VulkanDeviceComponent(this, registry_ptr);

    if(!dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER))->Get<bool>("defer-renderchain-initialisation", false)) {
        swapchain = new VulkanSwapchainComponent(this, registry_ptr);
        renderchain = new VulkanRenderchainComponent(this, registry_ptr);
    }

    DM().Log("Finished graphics backend bootstrapping in " + std::to_string(bootstrapTimer.delta()) + " milliseconds. Awaiting frame draw command");
}

VulkanHandlerIMPL::~VulkanHandlerIMPL() {
    if(device) {
        if(device->getDevice() != VK_NULL_HANDLE) vkDeviceWaitIdle(device->getDevice());
    }

    if(renderchain) delete renderchain;
    if(swapchain) delete swapchain;
    if(device) delete device;
    if(environment) delete environment;
}

//VulkanRuntimeEnvironment
    int VulkanRuntimeEnvironmentComponent::CreateVulkanInstance() {
        AlcInstanceCreateInfo CreateInfo{};
        FetchCreateData(CreateInfo);

        VkResult hold = vkCreateInstance(CreateInfo.Get(), nullptr, &Instance);
        if(hold != VK_SUCCESS) {
            throw AlcEngineException("Failed to create Vulkan instance! " + std::to_string(hold));
            
        } else {
            DM().Log("Successfully constructed Vulkan instance");
            return 0;
        }
    }

    void VulkanRuntimeEnvironmentComponent::FetchCreateData(AlcInstanceCreateInfo& ReturnBundle) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

        FetchDebugData(ReturnBundle._pNext);
        ReturnBundle._flags = 0;
        FetchAppData(ReturnBundle._pApplicationInfo);
        if(CM->Get<bool>("debug", true)) {
            ReturnBundle._ppEnabledLayerNames = CM->Get<std::vector<std::string>>("debug_layers", {"VK_LAYER_KHRONOS_validation"});
        } else {
            ReturnBundle._ppEnabledLayerNames;
        }
        ReturnBundle._ppEnabledExtensionNames = CM->Get<std::vector<std::string>>("extensions", {});
        ReturnBundle._ppEnabledExtensionNames.push_back("VK_EXT_debug_utils");
    }

    void VulkanRuntimeEnvironmentComponent::FetchAppData(AlcApplicationInfo& ReturnBundle) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

        ReturnBundle._apiVersion = VK_API_VERSION_1_4;
        ReturnBundle._pApplicationName = CM->Get<std::string>("application_name", "Default Application");
        ReturnBundle._pEngineName = CM->Get<std::string>("engine_name", "Alcubierre Engine");

        std::vector<int> appversion = CM->Get<std::vector<int>>("application_version", {0, 0, 0});
        ReturnBundle._applicationVersion = VK_MAKE_VERSION(static_cast<uint32_t>(appversion[0]), static_cast<uint32_t>(appversion[1]), static_cast<uint32_t>(appversion[2]));
        std::vector<int> engineversion = CM->Get<std::vector<int>>("engine_version", {0, 0, 0});
        ReturnBundle._engineVersion = VK_MAKE_VERSION(static_cast<uint32_t>(engineversion[0]), static_cast<uint32_t>(engineversion[1]), static_cast<uint32_t>(engineversion[2]));
    }

    int VulkanRuntimeEnvironmentComponent::CreateDebugLink() {
        AlcDebugUtilsMessengerCreateInfoEXT DebugLinkCreateInfo{};
        FetchDebugData(DebugLinkCreateInfo);

        PFN_vkCreateDebugUtilsMessengerEXT CreateFunction = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
        if(CreateFunction != nullptr) {
            VkResult CreateStatus = CreateFunction(Instance, DebugLinkCreateInfo.Get(), nullptr, &DebugMessenger);
            if(CreateStatus != VK_SUCCESS) {
                throw VulkanException("Issue creating debug callback loop"); 
            } else {
                DM().Log("Successfully established Vulkan debug link");
                return 0;
            }
        } else {
            DM().Log("Debug extension not present, Vulkan failed to link"); 
            return 1;
        }
    }

    void VulkanRuntimeEnvironmentComponent::FetchDebugData(AlcDebugUtilsMessengerCreateInfoEXT& ReturnBundle) {
        ReturnBundle._messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        ReturnBundle._messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        ReturnBundle._pfnUserCallback = VulkanDebugCallback;
        ReturnBundle._pUserData = nullptr;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRuntimeEnvironmentComponent::VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::string CallbackMessage(pCallbackData->pMessage);

        DebugReport Report(
            CallbackMessage,
            1,
            "Vulkan Debug Callback",
            std::time(nullptr)
        );

        DM().Log(Report);

        return VK_FALSE;
    }

    int VulkanRuntimeEnvironmentComponent::CreateSurface() {
        int hold = static_cast<IWindowSurfaceBridge*>(registry_ptr->FetchService(WINDOW_SURFACE))->CreateWindowSurface(&Instance, &Surface);

        if(hold == 1) {
            DM().Log("Successfully established Vulkan window surface bridge link");
        } else {
            throw VulkanException("Failed to establish Vulkan wsb link");
        }

        return hold;
    }

    VulkanRuntimeEnvironmentComponent::~VulkanRuntimeEnvironmentComponent() {
        if(Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(Instance, Surface, nullptr);
            Surface = VK_NULL_HANDLE;
            DM().Log("Successfully destroyed Vulkan surface");
        }

        try {
            PFN_vkDestroyDebugUtilsMessengerEXT DestroyFunction = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
            if(DestroyFunction != nullptr) {
                DestroyFunction(Instance, DebugMessenger, nullptr);
                DM().Log("Successfully destroyed Vulkan debug link");
            }
        } catch (...) {/*Do nothing for now*/}

        if(Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(Instance, nullptr);
            Instance = VK_NULL_HANDLE;
            DM().Log("Successfully destroyed Vulkan instance");
        }
    }

//VulkanDevice

    int VulkanDeviceComponent::CreateLogicalDevice() {
        PhysicalDevice = SelectPhysicalDevice();

        AlcDeviceCreateInfo DeviceInitInfo;
        FetchDeviceInfo(DeviceInitInfo);

        VkResult CreateEcho = vkCreateDevice(PhysicalDevice, DeviceInitInfo.Get(), nullptr, &Device);

        vkGetDeviceQueue(Device, GraphicsQueue.index, 0, &GraphicsQueue.queue);
        vkGetDeviceQueue(Device, SurfacePresentQueue.index, 0, &SurfacePresentQueue.queue);

        if(CreateEcho != VK_SUCCESS) {
            switch(CreateEcho) {
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                    throw VulkanException("Requested extension not present in device marked compatible.");
                break;

                case VK_ERROR_FEATURE_NOT_PRESENT:
                    throw VulkanException("Requested feature not present in device marked compatible.");
                break;

                default:
                    throw VulkanException("Failed to intialise logical device.");
                break;
            }

            return 1;

        } else {
            DM().Log("Successfully instantiated Vulkan logical device");

            return 0;
        }
    }

    VkPhysicalDevice VulkanDeviceComponent::SelectPhysicalDevice() {

        uint32_t CompatibleDeviceCount = 0;
        vkEnumeratePhysicalDevices(parent->environment->getInstance(), &CompatibleDeviceCount, nullptr);
        if(CompatibleDeviceCount == 0) throw std::runtime_error("No detected GPU devices support Vulkan.");
        std::vector<VkPhysicalDevice> CompatibleDevices(CompatibleDeviceCount);
        vkEnumeratePhysicalDevices(parent->environment->getInstance(), &CompatibleDeviceCount, CompatibleDevices.data());

        int u = 0;
        VkPhysicalDevice hold;
        for(const VkPhysicalDevice& device : CompatibleDevices) {
            int score = ScoreDevice(device);
            if(score > u) {
                u = score;
                hold = device;
            }
        }

        return hold;
    }

    int VulkanDeviceComponent::ScoreDevice(VkPhysicalDevice _PhysicalDevice) {    
        
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(_PhysicalDevice, &DeviceProperties);

        int hold = 0;

        //Required and preferred properties go here. Required = return 0 if not present. Preferred = add to score.
        if(DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            hold += 10000;
        }

        hold += DeviceProperties.apiVersion;

        VkPhysicalDeviceFeatures AvailableFeatures;
        vkGetPhysicalDeviceFeatures(_PhysicalDevice, &AvailableFeatures);

        uint32_t DeviceQueueFamiliesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &DeviceQueueFamiliesCount, nullptr);
        std::vector<VkQueueFamilyProperties> DeviceQueueFamilies(DeviceQueueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &DeviceQueueFamiliesCount, DeviceQueueFamilies.data());

        //We need to confirm, firstly, that a device *has* all the features we need to begin with. If it doesn't exclude it. If it does we can score it based on the quantity of supporting queues.
        bool _hasGraphics = false;
        bool _hasSurfaceSupport = false;

        for(int i = 0; i < DeviceQueueFamilies.size(); i++) {
            if(DeviceQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                hold += 100 * DeviceQueueFamilies[i].queueCount;
                _hasGraphics = true;
            }

            VkBool32 SurfaceSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(_PhysicalDevice, i, parent->environment->getSurface(), &SurfaceSupport);
            if(SurfaceSupport) _hasSurfaceSupport = true;
        }

        if(!(_hasGraphics && _hasSurfaceSupport)) {
            return 0;
        }

        return hold;
    }

    void VulkanDeviceComponent::FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle) {
        ReturnBundle._flags = 0;
        FetchQueueArray(ReturnBundle.queueCreateInfos);
        FetchDeviceExtensionArray(ReturnBundle._ppEnabledExtensionNames);
        FetchDeviceFeatures(ReturnBundle._features);
    }

    void VulkanDeviceComponent::FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
        int RequestedQueues = CM->Get<int>(std::vector<std::string>{"required_graphics_queues", "SIZE_T"}, 0);

        if(RequestedQueues == 0) {
            DM().Log("Applying default graphics queue settings");

            //Graphics queue
            CM->Set<std::vector<int>>({"required_graphics_queues", "0", "flags"}, "[1]");
            CM->Set<int>({"required_graphics_queues", "0", "count"}, "1");
            CM->Set<int>({"required_graphics_queues", "0", "priority"}, "1");

            //Surface queue
            CM->Set<bool>({"required_graphics_queues", "1", "surface_support"}, "true");
            CM->Set<int>({"required_graphics_queues", "1", "count"}, "1");
            CM->Set<int>({"required_graphics_queues", "1", "priority"}, "1");

            RequestedQueues = 2;
        }

        std::unordered_map<uint32_t, VkQueueFlags> AvailableQueues;
        
        uint32_t QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

        for (int i = 0; i < RequestedQueues; i++) {
            AlcDeviceQueueCreateInfo& queueCreateInfo = ReturnArray.emplace_back(AlcDeviceQueueCreateInfo());

            int UsableQueue = -1;
            std::vector<int> RequiredFlags = CM->Get<std::vector<int>>({"required_graphics_queues", std::to_string(i), "flags"}, {});
            bool SurfaceRequirement = CM->Get<bool>({"required_graphics_queues", std::to_string(i), "surface_support"}, false);
            for(int j = 0; j < QueueFamilies.size(); j++) {
                bool validQueue = true;

                for(int flag : RequiredFlags) {
                    if(!(QueueFamilies[j].queueFlags & flag)) validQueue = false;
                }

                if(SurfaceRequirement) {
                    VkBool32 SurfaceSupport;
                    vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, j, parent->environment->getSurface(), &SurfaceSupport);
                    if(!SurfaceSupport) validQueue = false;
                }

                if(validQueue) UsableQueue = j;
            }

            if(UsableQueue == -1) throw VulkanException("No available graphics queue fulfilled all requirements for queue: " + std::to_string(i + 1));

            queueCreateInfo._queueFamilyIndex = static_cast<uint32_t>(UsableQueue);
            if(SurfaceRequirement) {
                SurfacePresentQueue.index = static_cast<uint32_t>(UsableQueue);
            } else {
                GraphicsQueue.index = static_cast<uint32_t>(UsableQueue);
            }

            queueCreateInfo._queueCount = static_cast<uint32_t>(CM->Get<int>({"required_graphics_queues", std::to_string(i), "count"}, 1));
            queueCreateInfo._pQueuePriorities = static_cast<float>(CM->Get<int>({"required_graphics_queues", std::to_string(i), "priority"}, 1));
        }
    }

    void VulkanDeviceComponent::FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

        //Stuff other modules need.
        std::vector<std::string> requestedExtensions = CM->Get<std::vector<std::string>>("required_device_extensions", {});

        //Stuff Vulkan needs.
        requestedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        requestedExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        requestedExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);

        //Duplicates are gracefully ignored. Source: I made it the fuck up, shit man I really hope they are.
        for(std::string extension : requestedExtensions) {
            ReturnArray.emplace_back(extension);
        }
    }

    void VulkanDeviceComponent::FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle) {
        // ReturnBundle.featuresBase.geometryShader = VK_TRUE;

        ReturnBundle.featuresVk1_3.dynamicRendering = VK_TRUE;
        ReturnBundle.featuresVk1_1.shaderDrawParameters = VK_TRUE;
        ReturnBundle.featuresDynamicState.extendedDynamicState = VK_TRUE;
        ReturnBundle.featuresVk1_3.synchronization2 = VK_TRUE; //For dynamic rendering
    }

    VulkanDeviceComponent::~VulkanDeviceComponent() {
        if(Device != VK_NULL_HANDLE) {
            vkDestroyDevice(Device, nullptr);
            Device = VK_NULL_HANDLE;
            DM().Log("Successfully destroyed Vulkan logical device");
        }
    }

//VulkanSwapchain
    int VulkanSwapchainComponent::CreateSwapchain() {
        VkSwapchainCreateInfoKHR ChainInitInfo {};
        ChainInitInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(parent->device->getPhysicalDevice(), parent->environment->getSurface(), &surfaceCapabilities);

        IWindowManager::WindowInfo* info = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER))->GetWindowInfo();
        frameWidth = std::clamp<uint32_t>(static_cast<uint32_t>(info->width_pix), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        frameHeight = std::clamp<uint32_t>(static_cast<uint32_t>(info->height_pix), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
        frameExtent = { .width=frameWidth, .height=frameHeight };

        ChainInitInfo.imageExtent = frameExtent;
        ChainInitInfo.imageArrayLayers = 1;
        ChainInitInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ChainInitInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
        ChainInitInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ChainInitInfo.preTransform = surfaceCapabilities.currentTransform;

        FetchSwapMode(ChainInitInfo.presentMode);
        FetchSwapSurfaceFormat(ChainInitInfo.imageFormat, ChainInitInfo.imageColorSpace);

        ChainInitInfo.surface = parent->environment->getSurface();

        VkResult hold = vkCreateSwapchainKHR(parent->device->getDevice(), &ChainInitInfo, nullptr, &Swapchain);

        if(hold == VK_SUCCESS) {
            uint32_t s;
            vkGetSwapchainImagesKHR(parent->device->getDevice(), Swapchain, &s, nullptr);
            DM().Log("Successfully created Vulkan swapchain consisting of " + std::to_string(s) + " images");
            return FillSwapchainImages(ChainInitInfo);
        } else {
            throw VulkanException("Failed to create Vulkan swapchain due to VK error: " + std::to_string(hold));
        }
    }

    int VulkanSwapchainComponent::FillSwapchainImages(VkSwapchainCreateInfoKHR& ChainInitInfo) {
        ChainFormat = ChainInitInfo.imageFormat;

        uint32_t s;
        vkGetSwapchainImagesKHR(parent->device->getDevice(), Swapchain, &s, nullptr);
        ChainImages.resize(s);
        vkGetSwapchainImagesKHR(parent->device->getDevice(), Swapchain, &s, ChainImages.data());

        VkImageSubresourceRange ImageSubresourceRange {};
        ImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageSubresourceRange.baseMipLevel = 0;
        ImageSubresourceRange.levelCount = 1;
        ImageSubresourceRange.baseArrayLayer = 0;
        ImageSubresourceRange.layerCount = 1;

        int successes = 0;
        for(int i = 0; i < ChainImages.size(); i++) {
            ChainImageViews.push_back(VkImageView());

            VkImageViewCreateInfo ImageViewCreateInfo {};
            ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

            ImageViewCreateInfo.image = ChainImages[i];
            ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ImageViewCreateInfo.format = ChainFormat;
            ImageViewCreateInfo.subresourceRange = ImageSubresourceRange;

            VkResult hold = vkCreateImageView(parent->device->getDevice(), &ImageViewCreateInfo, nullptr, &ChainImageViews[i]);

            if(hold != VK_SUCCESS) DM().Log("Failed to acquire image view on index [" + std::to_string(i) + "]");
            else successes++;
        }

        if(successes > 0) return 0;
        else throw VulkanException("No image views were able to be acquired.");
    }

    void VulkanSwapchainComponent::FetchSwapMode(VkPresentModeKHR& ReturnMode) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
        
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(parent->device->getPhysicalDevice(), parent->environment->getSurface(), &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(parent->device->getPhysicalDevice(), parent->environment->getSurface(), &presentModeCount, presentModes.data());

        std::vector<int> PreferredMode = CM->Get<std::vector<int>>(std::vector<std::string>{"graphics", "presentation_mode"}, std::vector<int>{VK_PRESENT_MODE_FIFO_KHR});
        for(int i = 0; i < PreferredMode.size(); i++) {
            for(const VkPresentModeKHR& mode : presentModes) {
                if(PreferredMode[i] == mode) {
                    ReturnMode = mode;
                    return;
                }
            }
        }

        throw VulkanException("No available present mode was suitable.");
    }

    void VulkanSwapchainComponent::FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(parent->device->getPhysicalDevice(), parent->environment->getSurface(), &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(parent->device->getPhysicalDevice(), parent->environment->getSurface(), &surfaceFormatCount, surfaceFormats.data());

        std::vector<int> PreferredFormat = CM->Get<std::vector<int>>(std::vector<std::string>{"graphics", "image_format"}, std::vector<int>{VK_FORMAT_B8G8R8A8_SRGB});
        for(int i = 0; i < PreferredFormat.size(); i++) {
            for(const VkSurfaceFormatKHR& imageFormat : surfaceFormats) {
                if(PreferredFormat[i] == imageFormat.format) {
                    ReturnFormat = imageFormat.format;
                    ReturnColor = imageFormat.colorSpace;
                    return;
                }
            }
        }

        //Just use any format
        ReturnFormat = surfaceFormats[0].format;
        ReturnColor = surfaceFormats[0].colorSpace;
        DM().Log("No requested image format was available");
    }

    VulkanSwapchainComponent::~VulkanSwapchainComponent() {
        for(VkImageView view : ChainImageViews) {
            vkDestroyImageView(parent->device->getDevice(), view, nullptr);
            view = VK_NULL_HANDLE;
        }
        ChainImages.clear();
        vkDestroySwapchainKHR(parent->device->getDevice(), Swapchain, nullptr);
        Swapchain = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed swapchain");
    }

//VulkanRenderchain
    #include <fstream>

    int VulkanRenderchainComponent::CreateGraphicsPipeline() {
        std::ifstream file("basic_shader.spv", std::ios::ate | std::ios::binary);
        if(!file.is_open()) throw VulkanException("Failed to open shader file");
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        std::vector<uint32_t> shader(fileSize / sizeof(uint32_t));
        if (!file.read(reinterpret_cast<char*>(shader.data()), static_cast<std::streamsize>(fileSize))) {
            throw VulkanException("Failed to read shader");
        }
        file.close();

        VkShaderModuleCreateInfo shaderCreateInfo {};
        shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        shaderCreateInfo.codeSize = shader.size() * sizeof(uint32_t);
        shaderCreateInfo.pCode = shader.data();

        VkShaderModule basicShader;
        vkCreateShaderModule(parent->device->getDevice(), &shaderCreateInfo, nullptr, &basicShader);

        VkGraphicsPipelineCreateInfo pipeCreateInfo {};
        pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &parent->swapchain->getSwapFormat();
        pipeCreateInfo.pNext = &pipelineRenderingCreateInfo;

        std::vector<AlcPipelineShaderStageCreateInfo> shaderStageCreateInfos_arr; 
        FetchShaderStageCreateInfos(shaderStageCreateInfos_arr, basicShader);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
        shaderStageCreateInfos.reserve(shaderStageCreateInfos_arr.size());
        for(AlcPipelineShaderStageCreateInfo& shaderStage : shaderStageCreateInfos_arr) {
            shaderStageCreateInfos.push_back(*shaderStage.Get());
        }

        pipeCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
        pipeCreateInfo.pStages = shaderStageCreateInfos.data();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeCreateInfo.pVertexInputState = &vertexInputInfo;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipeCreateInfo.pInputAssemblyState = &inputAssemblyInfo;

        VkPipelineViewportStateCreateInfo viewportInfo {};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.scissorCount = 1;
        pipeCreateInfo.pViewportState = &viewportInfo;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo {};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.depthClampEnable = VK_FALSE;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationInfo.depthBiasEnable = VK_FALSE;
        rasterizationInfo.depthBiasSlopeFactor = 1.0f;
        rasterizationInfo.lineWidth = 1.0f;
        pipeCreateInfo.pRasterizationState = &rasterizationInfo;

        VkPipelineMultisampleStateCreateInfo multisampleInfo {};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleInfo.sampleShadingEnable = VK_FALSE;
        pipeCreateInfo.pMultisampleState = &multisampleInfo;

        VkPipelineColorBlendAttachmentState colorAttachment {};
        colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.attachmentCount = 1;
        colorBlendInfo.pAttachments = &colorAttachment;
        pipeCreateInfo.pColorBlendState = &colorBlendInfo;

        VkPipelineDynamicStateCreateInfo dynamicStateInfo {};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();
        pipeCreateInfo.pDynamicState = &dynamicStateInfo;

        VkPipelineLayoutCreateInfo layoutInfo {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 0;
        layoutInfo.pushConstantRangeCount = 0;
        vkCreatePipelineLayout(parent->device->getDevice(), &layoutInfo, nullptr, &pipeCreateInfo.layout);

        pipeCreateInfo.renderPass = VK_NULL_HANDLE;

        VkResult hold = vkCreateGraphicsPipelines(parent->device->getDevice(), nullptr, 1, &pipeCreateInfo, nullptr, &Pipeline);

        if(hold == VK_SUCCESS) {
            DM().Log("Successfully created graphics pipeline");
            return 0;
        } else {
            DM().Log("Failed to create Vulkan graphics pipeline due to error: " + std::to_string(hold));
            return 1;
        }
    }

    void VulkanRenderchainComponent::FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule) {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

            //Hacky stuff until I set it up proper
            CM->Set<int>({"shaders", "stages", "1", "bit"}, std::to_string(VK_SHADER_STAGE_VERTEX_BIT));
            CM->Set<std::string>({"shaders", "stages", "1", "name"}, "\"vertMain\"");
            CM->Set<int>({"shaders", "stages", "2", "bit"}, std::to_string(VK_SHADER_STAGE_FRAGMENT_BIT));
            CM->Set<std::string>({"shaders", "stages", "2", "name"}, "\"fragMain\"");

        int requiredShaderStages = CM->Get<int>(std::vector<std::string>{"shaders", "stages", "SIZE_T"}, 0);
        ReturnBundlesArray.reserve(requiredShaderStages);

        for(int i = 0; i < requiredShaderStages; i++) {
            AlcPipelineShaderStageCreateInfo& shaderStage = ReturnBundlesArray.emplace_back(AlcPipelineShaderStageCreateInfo());
            shaderStage._flags = 0;
            shaderStage._stage = static_cast<VkShaderStageFlagBits>(CM->Get<int>({"shaders", "stages", std::to_string(i + 1), "bit"}, VK_SHADER_STAGE_VERTEX_BIT));
            shaderStage._module = shaderModule;
            shaderStage._pName = CM->Get<std::string>({"shaders", "stages", std::to_string(i + 1), "name"}, "ERR");
        }
    }

    int VulkanRenderchainComponent::CreateCommandPool() {
        AlcCommandPoolCreateInfo CreateInfo {};
        GetCommandPoolCreateInfo(CreateInfo);

        VkResult hold = vkCreateCommandPool(parent->device->getDevice(), CreateInfo.Get(), nullptr, &CommandPool);

        if(hold == VK_SUCCESS) {
            DM().Log("Successfully created command pool");
        } else throw VulkanException("Failed to create command pool");

        AllocateFences(fences_draw, max_frames_in_flight);
        DM().Log("Successfully allocated fence signals");
        return 0;
    }

    void VulkanRenderchainComponent::GetCommandPoolCreateInfo(AlcCommandPoolCreateInfo& ReturnBundle) {
        ReturnBundle._flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ReturnBundle._queueFamilyIndex = parent->device->getGraphicsQueue().index;
    }

    int VulkanRenderchainComponent::CreateCommandBuffers() {
        max_frames_in_flight = static_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER))->Get<int>(std::vector<std::string>{"graphics", "max_frames_in_flight"}, 2);

        CommandBuffers.clear();

        AlcCommandBufferCreateInfo CreateInfo {};
        GetCommandBufferCreateInfo(CreateInfo);

        CommandBuffers.resize(CreateInfo.Get()->commandBufferCount);
        VkResult hold = vkAllocateCommandBuffers(parent->device->getDevice(), CreateInfo.Get(), CommandBuffers.data());

        if(hold == VK_SUCCESS) {
            DM().Log("Successfully created " + std::to_string(max_frames_in_flight) + " command buffers");
            return 0;
        } else throw VulkanException("Failed to create command buffers");
    }

    void VulkanRenderchainComponent::GetCommandBufferCreateInfo(AlcCommandBufferCreateInfo& ReturnBundle) {
        ReturnBundle._commandPool = CommandPool;
        ReturnBundle._level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ReturnBundle._commandBufferCount = max_frames_in_flight;
    }

    //Temporary proof of concept. This should be bundled into some kind of structure where assets can be arranged in 3D space and then rendered but for now we just want to render SOMETHING
    int VulkanRenderchainComponent::RecordCommandBuffer(VkCommandBuffer& CommandBuffer, int imageIndex) {
        VkImage& chainImage = parent->swapchain->getChainImage(imageIndex);
        VkImageView chainImageView = parent->swapchain->getChainImageView(imageIndex);

        //We may want to transition the image from and into any number of a given formats. Frequently used formats can be defined up here for convenience
        //We also have to pass the *current* format when transitioning, so make sure to store that too
        AlcImageLayoutDetails layer_undefined (VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, {}, VK_IMAGE_LAYOUT_UNDEFINED);
        AlcImageLayoutDetails layer_colorAttachment(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        AlcImageLayoutDetails layer_present(VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, {}, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        AlcImageLayoutDetails* context = &layer_undefined;

        VkCommandBufferBeginInfo BeginInfo { 
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = NULL_BIT,
            .pInheritanceInfo = nullptr
        };
        vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
        
        TransitionImageLayout(CommandBuffer, chainImage, *context, layer_colorAttachment);
        context = &layer_colorAttachment;

        VkClearValue _clearValue = { .color = VkClearColorValue({0.0f, 0.0f, 0.0f, 1.0f}) };

        VkRenderingAttachmentInfo colorRenderingAttachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = chainImageView,
            .imageLayout = layer_colorAttachment.layout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = _clearValue
        };

        VkRenderingInfo RenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = NULL_BIT,
            .renderArea = { .offset = {0,0}, .extent = parent->swapchain->getSwapExtent() },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRenderingAttachment,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
        };

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

        VkViewport viewport = VkViewport(0.0f, 0.0f, static_cast<float>(parent->swapchain->getSwapExtent().width), static_cast<float>(parent->swapchain->getSwapExtent().height), 0.0f, 1.0f);
        vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

        VkRect2D scissor = VkRect2D(VkOffset2D(0.0f, 0.0f), parent->swapchain->getSwapExtent());
        vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

        vkCmdDraw(CommandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(CommandBuffer);

        TransitionImageLayout(CommandBuffer, chainImage, *context, layer_present);
        context = &layer_present;

        vkEndCommandBuffer(CommandBuffer);

        return 0;
    }

    int VulkanRenderchainComponent::TransitionImageLayout(VkCommandBuffer& CommandBuffer, VkImage& _image, AlcImageLayoutDetails& _oldDetails, AlcImageLayoutDetails& _newDetails) {
        VkImageMemoryBarrier2 imageMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = _oldDetails.stageMask,
            .srcAccessMask = _oldDetails.accessMask,
            .dstStageMask = _newDetails.stageMask,
            .dstAccessMask = _newDetails.accessMask,
            .oldLayout = _oldDetails.layout,
            .newLayout = _newDetails.layout,
            .srcQueueFamilyIndex = _oldDetails.queueIndex,
            .dstQueueFamilyIndex = _newDetails.queueIndex,
            .image = _image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkDependencyInfo dependencyInfo {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageMemoryBarrier
        };

        vkCmdPipelineBarrier2(CommandBuffer, &dependencyInfo);

        return 0;
    }

    int VulkanRenderchainComponent::AllocateSemaphores(std::vector<VkSemaphore>& semaphores, int count) {
        VkSemaphoreCreateInfo CreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        semaphores.clear();
        semaphores.resize(count);
        for(int i = 0; i < count; i++) {
            VkResult hold = vkCreateSemaphore(parent->device->getDevice(), &CreateInfo, nullptr, &semaphores[i]);
            if(hold != VK_SUCCESS) throw VulkanException("Failed to allocate semaphore at index " + std::to_string(i) + " out of " + std::to_string(count));
        }

        return 0;
    }

    int VulkanRenderchainComponent::AllocateFences(std::vector<VkFence>& fences, int count) {
        VkFenceCreateInfo CreateInfo { 
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        fences.clear();
        fences.resize(count);
        for(int i = 0; i < count; i++) {
            VkResult hold = vkCreateFence(parent->device->getDevice(), &CreateInfo, nullptr, &fences[i]);
            if(hold != VK_SUCCESS) throw VulkanException("Failed to allocate fence at index " + std::to_string(i) + " out of " + std::to_string(count));
        }

        return 0;
    }

    VulkanRenderchainComponent::~VulkanRenderchainComponent() {
        for(VkSemaphore semaphore : semaphores_presentComplete) {
            if(semaphore) vkDestroySemaphore(parent->device->getDevice(), semaphore, nullptr);
        }
        for(VkSemaphore semaphore : semaphores_renderFinished) {
            if(semaphore) vkDestroySemaphore(parent->device->getDevice(), semaphore, nullptr);
        }
        for(VkFence fence : fences_draw) {
            if(fence) vkDestroyFence(parent->device->getDevice(), fence, nullptr);
        }

        if(parent->device->getDevice() != VK_NULL_HANDLE, CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(parent->device->getDevice(), CommandPool, nullptr);
            DM().Log("Successfully destroyed command pool");
        }

        if((parent->device->getDevice() != VK_NULL_HANDLE) && (Pipeline != VK_NULL_HANDLE)) {
            vkDestroyPipeline(parent->device->getDevice(), Pipeline, nullptr);
            DM().Log("Successfully destroyed graphics pipeline");
        }
    }

const uint64_t t_second = 1000000000; //qty of nanoseconds in 1 second
const uint64_t TIMEOUT_SET = 1; //seconds
uint64_t TIMEOUT = TIMEOUT_SET * t_second;
// uint64_t TIMEOUT = UINT64_MAX;
int currentFrame = 0;
int max_frames_in_flight = 2;

void VulkanHandlerIMPL::drawFrame() {
    vkWaitForFences(device->getDevice(), 1, &renderchain->getDrawingFence(currentFrame), VK_TRUE, TIMEOUT);

    uint32_t imageIndex;
    VkResult result_acquireNextImage = vkAcquireNextImageKHR(device->getDevice(), swapchain->getSwapchain(), TIMEOUT, renderchain->getPresentationSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex);

    vkResetFences(device->getDevice(), 1, &renderchain->getDrawingFence(currentFrame));
    vkResetCommandBuffer(renderchain->getCommandBuffer(currentFrame), NULL_BIT);

    renderchain->RecordCommandBuffer_(renderchain->getCommandBuffer(currentFrame), imageIndex);

    VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderchain->getPresentationSemaphore(currentFrame),
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &renderchain->getCommandBuffer(currentFrame),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderchain->getRenderSemaphore(currentFrame)
    };

    vkQueueSubmit(device->getGraphicsQueue().queue, 1, &SubmitInfo, renderchain->getDrawingFence(currentFrame));

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderchain->getRenderSemaphore(currentFrame),
        .swapchainCount = 1,
        .pSwapchains = &swapchain->getSwapchain(),
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    vkQueuePresentKHR(device->getSurfacePresentQueue().queue, &presentInfo);

    currentFrame = (currentFrame + 1) % max_frames_in_flight;
}