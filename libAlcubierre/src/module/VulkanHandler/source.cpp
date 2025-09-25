#include "module/VulkanHandler/public.h"
#include "module/VulkanHandler/private.h"

#include <utility>
#include <variant>

ModuleRegistryBundle VulkanHandlerWrapper::bundle(
    []() -> WrapperBaseClass* { return new VulkanHandlerWrapper(); },
    "MODULE_VULKANHANDLER"
);

VulkanHandler::VulkanHandler() : IGraphicsBackend_VulkanHandler(this) {}

VulkanHandler::~VulkanHandler() {
    if(PrivatePtr) delete PrivatePtr;
}

int VulkanHandler::WakeImpl() {
    if(!PrivatePtr) {
        PrivatePtr = new VulkanHandlerIMPL();
        return 1;
    } else {
        return 0;
    }
}

void* VulkanHandler::GetBackendObjectImpl() {
    return PrivatePtr;
}

VulkanHandlerIMPL::VulkanHandlerIMPL() {
    CreateVulkanInstance();
    CreateDebugLink();
    CreateSurface();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateGraphicsPipeline();
}

VulkanHandlerIMPL::~VulkanHandlerIMPL() {
    if(Device != VK_NULL_HANDLE) vkDeviceWaitIdle(Device);

    if(Pipeline != VK_NULL_HANDLE) {
        // delete Pipeline;
        // DM().Log("Successfully destroyed graphics pipeline");
    }

    if((Device != VK_NULL_HANDLE) && (Swapchain != VK_NULL_HANDLE)) {
        vkDestroySwapchainKHR(Device, Swapchain, nullptr);
        Swapchain = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed Vulkan swapchain");
    }

    if(Device != VK_NULL_HANDLE) {
        vkDestroyDevice(Device, nullptr);
        Device = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed Vulkan logical device");
    }

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

int VulkanHandlerIMPL::CreateVulkanInstance() {
    AlcInstanceCreateInfo CreateInfo{};
    FetchCreateData(CreateInfo);

    VkResult hold = vkCreateInstance(CreateInfo.Get(), nullptr, &Instance);
    if(hold != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance! " + std::to_string(hold));
        
    } else {
        DM().Log("Successfully constructed Vulkan instance");
        return 0;
    }
}

void VulkanHandlerIMPL::FetchCreateData(AlcInstanceCreateInfo& ReturnBundle) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    FetchDebugData(ReturnBundle._pNext);
    ReturnBundle._flags = 0;
    FetchAppData(ReturnBundle._pApplicationInfo);
    if(CM->Get<bool>("debug", false)) {
        ReturnBundle._ppEnabledLayerNames = CM->Get<std::vector<std::string>>("debug_layers", {"VK_LAYER_KHRONOS_validation"});
    } else {
        ReturnBundle._ppEnabledLayerNames;
    }
    ReturnBundle._ppEnabledExtensionNames = CM->Get<std::vector<std::string>>("extensions", {"VK_EXT_debug_utils"});
}

void VulkanHandlerIMPL::FetchAppData(AlcApplicationInfo& ReturnBundle) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    ReturnBundle._apiVersion = VK_API_VERSION_1_4;
    ReturnBundle._pApplicationName = CM->Get<std::string>("application_name", "Default Application");
    ReturnBundle._pEngineName = CM->Get<std::string>("engine_name", "Alcubierre Engine");

    std::vector<int> appversion = CM->Get<std::vector<int>>("application_version", {0, 0, 0});
    ReturnBundle._applicationVersion = VK_MAKE_VERSION(static_cast<uint32_t>(appversion[0]), static_cast<uint32_t>(appversion[1]), static_cast<uint32_t>(appversion[2]));
    std::vector<int> engineversion = CM->Get<std::vector<int>>("engine_version", {0, 0, 0});
    ReturnBundle._engineVersion = VK_MAKE_VERSION(static_cast<uint32_t>(engineversion[0]), static_cast<uint32_t>(engineversion[1]), static_cast<uint32_t>(engineversion[2]));
}

int VulkanHandlerIMPL::CreateDebugLink() {
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

void VulkanHandlerIMPL::FetchDebugData(AlcDebugUtilsMessengerCreateInfoEXT& ReturnBundle) {
    ReturnBundle._messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ReturnBundle._messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ReturnBundle._pfnUserCallback = VulkanDebugCallback;
    ReturnBundle._pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanHandlerIMPL::VulkanDebugCallback(
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

int VulkanHandlerIMPL::CreateSurface() {
    IWindowSurfaceBridge* WSB = dynamic_cast<IWindowSurfaceBridge*>(Registry::GetRegistry().FetchService("IWindowSurfaceBridge"));

    int hold = WSB->CreateWindowSurface(&Instance, &Surface);

    if(hold == 0) {
        DM().Log("Successfully established Vulkan window surface bridge link");
    } else {
        throw VulkanException("Failed to establish Vulkan wsb link");
    }

    return hold;
}

int VulkanHandlerIMPL::CreateLogicalDevice() {
    PhysicalDevice = SelectPhysicalDevice();

    AlcDeviceCreateInfo DeviceInitInfo;
    FetchDeviceInfo(DeviceInitInfo);

    VkResult CreateEcho = vkCreateDevice(PhysicalDevice, DeviceInitInfo.Get(), nullptr, &Device);

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

        const VkDeviceQueueCreateInfo* queues = DeviceInitInfo.Get()->pQueueCreateInfos;
        vkGetDeviceQueue(Device, queues[0].queueFamilyIndex, 0, &Q_List.GraphicsQueue);
        vkGetDeviceQueue(Device, queues[1].queueFamilyIndex, 0, &Q_List.SurfacePresentQueue);
        
        return 0;
    }
}

VkPhysicalDevice VulkanHandlerIMPL::SelectPhysicalDevice() {

    uint32_t CompatibleDeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &CompatibleDeviceCount, nullptr);
    if(CompatibleDeviceCount == 0) throw std::runtime_error("No detected GPU devices support Vulkan.");
    std::vector<VkPhysicalDevice> CompatibleDevices(CompatibleDeviceCount);
    vkEnumeratePhysicalDevices(Instance, &CompatibleDeviceCount, CompatibleDevices.data());

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

int VulkanHandlerIMPL::ScoreDevice(VkPhysicalDevice _PhysicalDevice) {    
    
    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(_PhysicalDevice, &DeviceProperties);

    VkPhysicalDeviceFeatures AvailableFeatures;
    vkGetPhysicalDeviceFeatures(_PhysicalDevice, &AvailableFeatures);

    int hold = 0;

    //Required and preferred properties go here. Required = return 0 if not present. Preferred = add to score.
    if(DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        hold += 10000;
    }

    hold += DeviceProperties.apiVersion;

    if(!AvailableFeatures.geometryShader) {
        return 0;
    }

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
        vkGetPhysicalDeviceSurfaceSupportKHR(_PhysicalDevice, i, Surface, &SurfaceSupport);
        if(SurfaceSupport) _hasSurfaceSupport = true;
    }

    if(!(_hasGraphics && _hasSurfaceSupport)) {
        return 0;
    }

    return hold;
}

void VulkanHandlerIMPL::FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle) {
    ReturnBundle._flags = 0;
    FetchQueueArray(ReturnBundle._pQueueCreateInfos);
    FetchDeviceExtensionArray(ReturnBundle._ppEnabledExtensionNames);
    ReturnBundle._pEnabledFeatures;
}

void VulkanHandlerIMPL::FetchQueueArray(std::vector<VkDeviceQueueCreateInfo>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));
    int RequestedQueues = CM->Get<std::vector<std::monostate>>("required_graphics_queues", {}).size();

    std::unordered_map<uint32_t, VkQueueFlags> AvailableQueues;
    
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    for (int i = 0; i < RequestedQueues; i++) {
        VkDeviceQueueCreateInfo hold{};
        hold.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

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
                vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, j, Surface, &SurfaceSupport);
                if(!SurfaceSupport) validQueue = false;
            }

            if(validQueue) UsableQueue = j;
        }

        if(UsableQueue == -1) throw VulkanException("No available graphics queue fulfilled all requirements for queue: " + std::to_string(i + 1));
        hold.queueFamilyIndex = static_cast<uint32_t>(UsableQueue);

        hold.queueCount = static_cast<uint32_t>(CM->Get<int>({"required_graphics_queues", std::to_string(i), "count"}, 1));

        float QueuePriority = static_cast<float>(CM->Get<int>({"required_graphics_queues", std::to_string(i), "priority"}, 1));
        hold.pQueuePriorities = &QueuePriority;

        ReturnArray.push_back(hold);
    }
}

void VulkanHandlerIMPL::FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

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

int VulkanHandlerIMPL::CreateSwapChain() {
    VkSwapchainCreateInfoKHR ChainInitInfo {};
    ChainInitInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &surfaceCapabilities);

    IWindowManager* WM = dynamic_cast<IWindowManager*>(Registry::GetRegistry().FetchService("IWindowManager"));
    uint32_t width = std::clamp<uint32_t>(static_cast<uint32_t>(WM->GetWindowInfo()->width_pix), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    uint32_t height = std::clamp<uint32_t>(static_cast<uint32_t>(WM->GetWindowInfo()->height_pix), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    ChainInitInfo.imageExtent = {width, height};

    ChainInitInfo.imageArrayLayers = 1;
    ChainInitInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ChainInitInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    ChainInitInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ChainInitInfo.preTransform = surfaceCapabilities.currentTransform;

    FetchSwapMode(ChainInitInfo.presentMode);
    FetchSwapSurfaceFormat(ChainInitInfo.imageFormat, ChainInitInfo.imageColorSpace);

    ChainInitInfo.surface = Surface;

    VkResult hold = vkCreateSwapchainKHR(Device, &ChainInitInfo, nullptr, &Swapchain);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created Vulkan swapchain");

        ChainFormat = ChainInitInfo.imageFormat;
        ChainExtent = ChainInitInfo.imageExtent;
        uint32_t i;
        vkGetSwapchainImagesKHR(Device, Swapchain, &i, nullptr);
        ChainImages.reserve(i);
        vkGetSwapchainImagesKHR(Device, Swapchain, &i, ChainImages.data());

        for(int i = 0; i < ChainImages.size(); i++) {
            ChainImageViews.push_back(new VkImageView());

            VkImageViewCreateInfo ImageViewCreateInfo {};
            ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

            ImageViewCreateInfo.image = ChainImages[i];
            ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ImageViewCreateInfo.format = ChainFormat;

            vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, ChainImageViews[i]);
        }

        return 0;
    } else {
        throw VulkanException("Failed to create Vulkan swapchain due to VK error: " + std::to_string(hold));
    }
}

void VulkanHandlerIMPL::FetchSwapMode(VkPresentModeKHR& ReturnMode) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, presentModes.data());

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

void VulkanHandlerIMPL::FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &surfaceFormatCount, surfaceFormats.data());

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

#include <fstream>

int VulkanHandlerIMPL::CreateGraphicsPipeline() {
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
    vkCreateShaderModule(Device, &shaderCreateInfo, nullptr, &basicShader);

    VkGraphicsPipelineCreateInfo pipeCreateInfo {};
    pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &ChainFormat;
    pipeCreateInfo.pNext = &pipelineRenderingCreateInfo;

    std::vector<AlcPipelineShaderStageCreateInfo> shaderStageCreateInfos_arr; 
    FetchShaderStageCreateInfos(shaderStageCreateInfos_arr, basicShader);
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(shaderStageCreateInfos_arr.size());
    for(AlcPipelineShaderStageCreateInfo& shaderStage : shaderStageCreateInfos_arr) {
        shaderStageCreateInfos.push_back(VkPipelineShaderStageCreateInfo(*shaderStage.Get()));
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

    VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.attachmentCount = 1;
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
    vkCreatePipelineLayout(Device, &layoutInfo, nullptr, &pipeCreateInfo.layout);

    pipeCreateInfo.renderPass = nullptr;

    VkResult hold = vkCreateGraphicsPipelines(Device, nullptr, 1, &pipeCreateInfo, nullptr, &Pipeline);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created graphics pipeline");
        return 0;
    } else {
        DM().Log("Failed to create Vulkan graphics pipeline due to error: " + std::to_string(hold));
        return 1;
    }
}

void VulkanHandlerIMPL::FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

        //Hacky stuff until I set it up proper
        CM->Set<int>({"shaders", "stages", "1", "bit"}, std::to_string(VK_SHADER_STAGE_VERTEX_BIT));
        CM->Set<std::string>({"shaders", "stages", "1", "name"}, "\"vertMain\"");
        CM->Set<int>({"shaders", "stages", "2", "bit"}, std::to_string(VK_SHADER_STAGE_FRAGMENT_BIT));
        CM->Set<std::string>({"shaders", "stages", "2", "name"}, "\"fragMain\"");

    int requiredShaderStages = CM->Get<std::vector<std::monostate>>(std::vector<std::string>{"shaders", "stages"}, {std::monostate{}}).size();
    ReturnBundlesArray.reserve(requiredShaderStages);

    for(int i = 0; i < requiredShaderStages; i++) {
        AlcPipelineShaderStageCreateInfo shaderStage {};
        shaderStage._flags = 0;
        shaderStage._stage = static_cast<VkShaderStageFlagBits>(CM->Get<int>({"shaders", "stages", std::to_string(i + 1), "bit"}, VK_SHADER_STAGE_VERTEX_BIT));
        shaderStage._module = shaderModule;
        shaderStage._pName = CM->Get<std::string>({"shaders", "stages", std::to_string(i + 1), "name"}, "ERR");
        ReturnBundlesArray.push_back(AlcPipelineShaderStageCreateInfo(shaderStage));
    }
}