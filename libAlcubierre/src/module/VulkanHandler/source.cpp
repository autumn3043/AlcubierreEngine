#include "module/VulkanHandler/public.h"
#include "module/VulkanHandler/private.h"

#include <utility>
#include <unordered_map>
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
}

VulkanHandlerIMPL::~VulkanHandlerIMPL() {
    vkDeviceWaitIdle(Device);

    if(Device && Swapchain) {
        vkDestroySwapchainKHR(Device, Swapchain, nullptr);
        Swapchain = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed Vulkan surface");
    }

    if(Device) {
        vkDestroyDevice(Device, nullptr);
        Device = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed Vulkan logical device");
    }

    if(Surface) {
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

    if(Instance) {
        vkDestroyInstance(Instance, nullptr);
        Instance = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed Vulkan instance");
    }
}

int VulkanHandlerIMPL::CreateVulkanInstance() {
    AlcInstanceCreateInfo CreateInfo{};
    FetchCreateData(CreateInfo);

    AlcDebugUtilsMessengerCreateInfoEXT DebugLinkCreateInfo{};
    FetchDebugData(DebugLinkCreateInfo);
    CreateInfo.Get()->pNext = (VkDebugUtilsMessengerCreateInfoEXT*) DebugLinkCreateInfo.Get();

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

    VkInstanceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    AlcApplicationInfo VkAppData;
    FetchAppData(VkAppData);
    hold.pApplicationInfo = VkAppData.Get();

    std::vector<std::string> InstanceExtensions = CM->Get<std::vector<std::string>>("extensions", {"VK_EXT_debug_utils"});
    hold.enabledExtensionCount = InstanceExtensions.size();
    std::vector<const char*> pInstanceExtensions;
    for(const std::string& str : InstanceExtensions) pInstanceExtensions.push_back(str.c_str());
    hold.ppEnabledExtensionNames = pInstanceExtensions.data();

    if(CM->Get<bool>("debug", false)) {
        std::vector<std::string> ValidationLayers = CM->Get<std::vector<std::string>>("debug_layers", {});
        hold.enabledLayerCount = ValidationLayers.size();
        std::vector<const char*> pValidationLayers;
        for(const std::string& str : ValidationLayers) pValidationLayers.push_back(str.c_str());
        hold.ppEnabledLayerNames = pValidationLayers.data();
    }
    
    ReturnBundle.Set(hold);
}

void VulkanHandlerIMPL::FetchAppData(AlcApplicationInfo& ReturnBundle) {
    VkApplicationInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    hold.pEngineName = CM->Get<std::string>("engine_name", "Alcubierre Engine").c_str();

    std::vector<int> engineversion = CM->Get<std::vector<int>>("engine_version", {0, 0, 0});
    hold.engineVersion = VK_MAKE_VERSION(static_cast<uint32_t>(engineversion[0]), static_cast<uint32_t>(engineversion[1]), static_cast<uint32_t>(engineversion[2]));

    hold.pApplicationName = CM->Get<std::string>("application_name", "Default Application").c_str();

    std::vector<int> appversion = CM->Get<std::vector<int>>("application_version", {0, 0, 0});
    hold.applicationVersion = VK_MAKE_VERSION(static_cast<uint32_t>(appversion[0]), static_cast<uint32_t>(appversion[1]), static_cast<uint32_t>(appversion[2]));


    hold.apiVersion = VK_API_VERSION_1_0;

    ReturnBundle.Set(hold);
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
    VkDebugUtilsMessengerCreateInfoEXT hold{};

    hold.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    hold.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    hold.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    hold.pfnUserCallback = VulkanDebugCallback;
    hold.pUserData = nullptr;

    ReturnBundle.Set(hold);
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
        hold += 1000;
    }

    //Same deal for features.
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
    VkDeviceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures DeviceFeatures{};
    hold.pEnabledFeatures = &DeviceFeatures;

    std::vector<std::string> DeviceExtensions;
    FetchDeviceExtensionArray(DeviceExtensions);
    hold.enabledExtensionCount = DeviceExtensions.size();
    std::vector<const char*> pDeviceExtensions;
    for(const std::string& str : DeviceExtensions) pDeviceExtensions.push_back(str.c_str());
    hold.ppEnabledExtensionNames = pDeviceExtensions.data();

    std::vector<VkDeviceQueueCreateInfo> QueueArray;
    FetchQueueArray(QueueArray);
    hold.pQueueCreateInfos = QueueArray.data();
    hold.queueCreateInfoCount = QueueArray.size();

    ReturnBundle.Set(hold);
}

void VulkanHandlerIMPL::FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    std::vector<std::string> requestedExtensions = CM->Get<std::vector<std::string>>("required_device_extensions", {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

    for(std::string extension : requestedExtensions) {
        ReturnArray.emplace_back(extension);
    }
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

int VulkanHandlerIMPL::CreateSwapChain() {
    VkSwapchainCreateInfoKHR ChainInitInfo {};
    ChainInitInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &surfaceCapabilities);

    FetchSwapMode(ChainInitInfo.presentMode);
    FetchSwapSurfaceFormat(ChainInitInfo.imageFormat, ChainInitInfo.imageColorSpace);

    ChainInitInfo.surface = Surface;

    VkResult hold = vkCreateSwapchainKHR(Device, &ChainInitInfo, nullptr, &Swapchain);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created Vulkan swapchain");
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