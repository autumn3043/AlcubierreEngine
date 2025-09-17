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
}

VulkanHandlerIMPL::~VulkanHandlerIMPL() {
    try {
        vkDestroyDevice(Device, nullptr);
        DM().Log("Successfully destroyed Vulkan logical device");
    } catch (...) {/*Do nothing for now*/}

    try {
        PFN_vkDestroyDebugUtilsMessengerEXT DestroyFunction = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
        if(DestroyFunction != nullptr) {
            DestroyFunction(Instance, DebugMessenger, nullptr);
            DM().Log("Successfully destroyed Vulkan debug link");
        }
    } catch (...) {/*Do nothing for now*/}

    try {
        vkDestroyInstance(Instance, nullptr);
        DM().Log("Successfully destroyed Vulkan instance");
    } catch (...) {
        DM().Log("Failed to destroy Vulkan instance");
    }
}

int VulkanHandlerIMPL::CreateVulkanInstance() {
    AlcInstanceCreateInfo CreateInfo{};
    FetchCreateData(CreateInfo);

        //Local addendums to CreateInfo because passing around a copy of this is a pain in the ass and well beyond the scope of the struct bundle system.
        AlcEnabledExtensions EnabledExtensions;
        FetchExtensionData(EnabledExtensions);
        CreateInfo.Get()->enabledExtensionCount = static_cast<uint32_t>(EnabledExtensions.Get()->size());
        CreateInfo.Get()->ppEnabledExtensionNames = EnabledExtensions.Get()->data();

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
    VkInstanceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    AlcApplicationInfo VkAppData;
    FetchAppData(VkAppData);
    hold.pApplicationInfo = VkAppData.Get();
    
    ReturnBundle.Set(hold);
}

void VulkanHandlerIMPL::FetchExtensionData(AlcEnabledExtensions& ReturnBundle) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));
    std::vector<std::string> EnabledExtensionsStr = CM->Get<std::vector<std::string>>("extensions", {"VK_EXT_debug_utils"});

    ReturnBundle.Set(EnabledExtensionsStr);
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
            DM().Log("Successfully established debug link");
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
    VkPhysicalDevice PhysicalDevice = SelectPhysicalDevice();

    AlcDeviceCreateInfo DeviceInitInfo;
    FetchDeviceInfo(PhysicalDevice, DeviceInitInfo);

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

    for(const VkQueueFamilyProperties& QueueFamily : DeviceQueueFamilies) {
        if(QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            hold += 100 * QueueFamily.queueCount;
            _hasGraphics = true;
        }
    }

    if(!(_hasGraphics)) {
        return 0;
    }

    return hold;
}

void VulkanHandlerIMPL::FetchDeviceInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceCreateInfo& ReturnBundle) {
    VkDeviceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    std::vector<VkDeviceQueueCreateInfo> QueueArray;
    FetchQueueArray(_PhysicalDevice, QueueArray);
    hold.pQueueCreateInfos = QueueArray.data(); //An array of queue create infos (pointer to first element)
    hold.queueCreateInfoCount = QueueArray.size(); //Count

    VkPhysicalDeviceFeatures DeviceFeatures{};
    hold.pEnabledFeatures = &DeviceFeatures;

    hold.enabledExtensionCount = 0;

    ReturnBundle.Set(hold);
}

void VulkanHandlerIMPL::FetchQueueArray(VkPhysicalDevice _PhysicalDevice, std::vector<VkDeviceQueueCreateInfo>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));
    int RequestedQueues = CM->Get<std::vector<std::monostate>>("required_graphics_queues", {}).size();

    std::unordered_map<uint32_t, VkQueueFlags> AvailableQueues;
    
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &QueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

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
                vkGetPhysicalDeviceSurfaceSupportKHR(_PhysicalDevice, j, Surface, &SurfaceSupport);
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