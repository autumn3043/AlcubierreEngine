//Space left for shorthand defines

VulkanEnvironmentComponent::VulkanEnvironmentComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    CreateVulkanInstance();
    CreateDebugLink();
    CreateSurface();
};

VulkanEnvironmentComponent::~VulkanEnvironmentComponent() {
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

int VulkanEnvironmentComponent::CreateVulkanInstance() {
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

void VulkanEnvironmentComponent::FetchCreateData(AlcInstanceCreateInfo& ReturnBundle) {
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

void VulkanEnvironmentComponent::FetchAppData(AlcApplicationInfo& ReturnBundle) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    ReturnBundle._apiVersion = VK_API_VERSION_1_4;
    ReturnBundle._pApplicationName = CM->Get<std::string>("application_name", "Default Application");
    ReturnBundle._pEngineName = CM->Get<std::string>("engine_name", "Alcubierre Engine");

    std::vector<int> appversion = CM->Get<std::vector<int>>("application_version", {0, 0, 0});
    ReturnBundle._applicationVersion = VK_MAKE_VERSION(static_cast<uint32_t>(appversion[0]), static_cast<uint32_t>(appversion[1]), static_cast<uint32_t>(appversion[2]));
    std::vector<int> engineversion = CM->Get<std::vector<int>>("engine_version", {0, 0, 0});
    ReturnBundle._engineVersion = VK_MAKE_VERSION(static_cast<uint32_t>(engineversion[0]), static_cast<uint32_t>(engineversion[1]), static_cast<uint32_t>(engineversion[2]));
}

int VulkanEnvironmentComponent::CreateDebugLink() {
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

void VulkanEnvironmentComponent::FetchDebugData(AlcDebugUtilsMessengerCreateInfoEXT& ReturnBundle) {
    ReturnBundle._messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ReturnBundle._messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ReturnBundle._pfnUserCallback = VulkanDebugCallback;
    ReturnBundle._pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEnvironmentComponent::VulkanDebugCallback(
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

int VulkanEnvironmentComponent::CreateSurface() {
    int hold = static_cast<IWindowSurfaceBridge*>(registry_ptr->FetchService(WINDOW_SURFACE))->CreateWindowSurface(&Instance, &Surface);

    if(hold == 1) {
        DM().Log("Successfully established Vulkan window surface bridge link");
    } else {
        throw VulkanException("Failed to establish Vulkan wsb link");
    }

    return hold;
}

//Undefine shorthands!!