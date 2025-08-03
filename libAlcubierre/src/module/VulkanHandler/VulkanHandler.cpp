#include "module/VulkanHandler/VulkanHandler.h"

#include <utility>
#include <algorithm>

VulkanHandler::VulkanHandler() {
    try {
        Init();
    } catch(...) {
        throw;
    }
}

VulkanHandler::~VulkanHandler() {
    try {
        vkDestroyDevice(Device, nullptr);
        DebugManager::Log("Successfully destroyed Vulkan logical device");
    } catch (...) {/*Do nothing for now*/}

    try {
        PFN_vkDestroyDebugUtilsMessengerEXT DestroyFunction = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
        if(DestroyFunction != nullptr) {
            DestroyFunction(Instance, DebugMessenger, nullptr);
            DebugManager::Log("Successfully destroyed Vulkan debug link");
        }
    } catch (...) {/*Do nothing for now*/}

    try {
        vkDestroyInstance(Instance, nullptr);
        DebugManager::Log("Successfully destroyed Vulkan instance");
    } catch (...) {
        DebugManager::Log("Failed to destroy Vulkan instance");
    }
}

int VulkanHandler::Init() {
    CreateVulkanInstance();
    CreateDebugLink();
    CreateLogicalDevice(0);
    return 0;
}

int VulkanHandler::CreateVulkanInstance() {
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

    if (vkCreateInstance(CreateInfo.Get(), nullptr, &Instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    } else {
        DebugManager::Log("Successfully constructed Vulkan instance");
        return 0;
    }
}

void VulkanHandler::FetchCreateData(AlcInstanceCreateInfo& ReturnBundle) {
    VkInstanceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    AlcApplicationInfo VkAppData;
    FetchAppData(VkAppData);
    hold.pApplicationInfo = VkAppData.Get();
    
    ReturnBundle.Set(hold);
}

void VulkanHandler::FetchExtensionData(AlcEnabledExtensions& ReturnBundle) {
    DataManager& DM = DataManager::GetDataManager();
    std::vector<std::string> EnabledExtensionsStr = DM.Get<std::vector<std::string>>("extensions");

    ReturnBundle.Set(EnabledExtensionsStr);
}

void VulkanHandler::FetchAppData(AlcApplicationInfo& ReturnBundle) {
    VkApplicationInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    DataManager& DM = DataManager::GetDataManager();

    hold.pEngineName = DM.Get<std::string>("engine_name").c_str();

    std::vector<int> engineversion = DM.Get<std::vector<int>>("engine_version");
    hold.engineVersion = VK_MAKE_VERSION(static_cast<uint32_t>(engineversion[0]), static_cast<uint32_t>(engineversion[1]), static_cast<uint32_t>(engineversion[2]));

    hold.pApplicationName = DM.Get<std::string>("application_name").c_str();

    std::vector<int> appversion = DM.Get<std::vector<int>>("application_version");
    hold.applicationVersion = VK_MAKE_VERSION(static_cast<uint32_t>(appversion[0]), static_cast<uint32_t>(appversion[1]), static_cast<uint32_t>(appversion[2]));


    hold.apiVersion = VK_API_VERSION_1_0;

    ReturnBundle.Set(hold);
}

int VulkanHandler::CreateDebugLink() {
    AlcDebugUtilsMessengerCreateInfoEXT DebugLinkCreateInfo{};
    FetchDebugData(DebugLinkCreateInfo);

    PFN_vkCreateDebugUtilsMessengerEXT CreateFunction = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    if(CreateFunction != nullptr) {
        VkResult CreateStatus = CreateFunction(Instance, DebugLinkCreateInfo.Get(), nullptr, &DebugMessenger);
        if(CreateStatus != VK_SUCCESS) {
            throw VulkanException("Issue creating debug callback loop"); 
        } else {
            DebugManager::Log("Successfully established debug link");
            return 0;
        }
    } else {
        DebugManager::Log("Debug extension not present, Vulkan failed to link"); 
        return 1;
    }
}

void VulkanHandler::FetchDebugData(AlcDebugUtilsMessengerCreateInfoEXT& ReturnBundle) {
    VkDebugUtilsMessengerCreateInfoEXT hold{};

    hold.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    hold.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    hold.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    hold.pfnUserCallback = VulkanDebugCallback;
    hold.pUserData = nullptr;

    ReturnBundle.Set(hold);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanHandler::VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::string CallbackMessage(pCallbackData->pMessage);


    AlcExceptions::DebugReport Report(
        CallbackMessage,
        "Vulkan Debug Callback",
        {},
        std::time(nullptr),
        1
    );

    return VK_FALSE;
}

int VulkanHandler::CreateLogicalDevice(int overrideIndice) {
    try {
        std::vector<std::pair<int, VkPhysicalDevice>> PhysicalDeviceCandidates = SelectPhysicalDevice();
        VkPhysicalDevice PhysicalDevice = std::get<1>(PhysicalDeviceCandidates[overrideIndice]);  

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
            DebugManager::Log("Successfully instantiated Vulkan logical device");

            vkGetDeviceQueue(Device, GetDeviceIndices(PhysicalDevice)[0], 0, &Queue);
            
            return 0;
        }

    } catch(...) {
        throw;
    }
}

std::vector<std::pair<int, VkPhysicalDevice>> VulkanHandler::SelectPhysicalDevice() {
    uint32_t CompatibleDeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &CompatibleDeviceCount, nullptr);
    if(CompatibleDeviceCount == 0) {
        throw std::runtime_error("No detected GPU devices support Vulkan.");
    }

    std::vector<VkPhysicalDevice> CompatibleDevices(CompatibleDeviceCount);
    vkEnumeratePhysicalDevices(Instance, &CompatibleDeviceCount, CompatibleDevices.data());

    //The double call here firstly checks a compatible device exists, before getting a list of all these compatible devices. There is no point scoring devices if none exist.

    std::vector<std::pair<int, VkPhysicalDevice>> Candidates;

    for(const VkPhysicalDevice& device : CompatibleDevices) {
        int score = ScoreDevice(device);
        if(score > 0) Candidates.emplace_back(std::make_pair(score, device));
    }

    std::sort(Candidates.begin(), Candidates.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    return Candidates;
}

int VulkanHandler::ScoreDevice(VkPhysicalDevice _PhysicalDevice) {    
    
    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(_PhysicalDevice, &DeviceProperties);

    VkPhysicalDeviceFeatures AvailableFeatures;
    vkGetPhysicalDeviceFeatures(_PhysicalDevice, &AvailableFeatures);

    int hold = 0;

    //Same deal for properties
    if(DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        hold += 1000;
    }

    //Required and preferred features go here. Required = return 0 if not present. Preferred = add to score.
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

void VulkanHandler::FetchDeviceInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceCreateInfo& ReturnBundle) {
    VkDeviceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    AlcDeviceQueueCreateInfo GraphicsQueueCreateInfo;
    FetchQueueInfo(_PhysicalDevice, GraphicsQueueCreateInfo);
    hold.pQueueCreateInfos = GraphicsQueueCreateInfo.Get();
    hold.queueCreateInfoCount = 1;

    VkPhysicalDeviceFeatures DeviceFeatures{};
    hold.pEnabledFeatures = &DeviceFeatures;

    hold.enabledExtensionCount = 0;

    ReturnBundle.Set(hold);
}

void VulkanHandler::FetchQueueInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceQueueCreateInfo& ReturnBundle) {
    VkDeviceQueueCreateInfo hold {};
    hold.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

    hold.queueFamilyIndex = GetDeviceIndices(_PhysicalDevice)[0];
    hold.queueCount = 1;

    float QueuePriority = 1.0f;
    hold.pQueuePriorities = &QueuePriority;

    ReturnBundle.Set(hold);
}

std::vector<uint32_t> VulkanHandler::GetDeviceIndices(VkPhysicalDevice _PhysicalDevice) {
    std::vector<uint32_t> hold;

    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    int i = 0;
    for (const auto& QueueFamily : QueueFamilies) {
        if(QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            hold.push_back(i);
        }

        i++;    
    }

    return hold;
}