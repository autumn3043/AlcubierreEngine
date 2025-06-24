#include "module/VulkanHandler/VulkanHandler.h"

#include "module/DataManager/DataManager.h"
#include "module/DebugManager/DebugManager.h"

#include <cstdlib>

VulkanHandler::VulkanHandler() {
    try {
        int _trouble = 0;
        _trouble += CreateVulkanInstance();
        if(_trouble > 0) throw AlcExceptions::SetupFail();
    } catch(...) {
        throw;
    }
}

VulkanHandler::~VulkanHandler() {
    vkDestroyDevice(Device, nullptr);
    vkDestroyInstance(Instance, nullptr);

    DebugManager::Log("Successfully destroyed Vulkan instance");
}

int VulkanHandler::CreateVulkanInstance() {
    VkInstanceCreateInfo CreateInfo = FetchCreateData();
    if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    } else {
        DebugManager::Log("Successfully constructed Vulkan instance");
        return 0;
    }
}

VkInstanceCreateInfo VulkanHandler::FetchCreateData() {
    VkInstanceCreateInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo VkAppData = FetchAppData();
    hold.pApplicationInfo = &VkAppData; //Two lines cuz out of scope copy otherwise ykwim

    hold.enabledExtensionCount = static_cast<uint32_t>(0);
    hold.ppEnabledExtensionNames = nullptr;
    return hold;
}

VkApplicationInfo VulkanHandler::FetchAppData() {
    VkApplicationInfo hold{};
    hold.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    DataManagerNamespace::appdata AppData = DataManager::GetDataManager().GetAppData();
    hold.pApplicationName = AppData.Name.c_str();
    hold.applicationVersion = VK_MAKE_VERSION(static_cast<uint32_t>(AppData.Version.GetIndex(0)), static_cast<uint32_t>(AppData.Version.GetIndex(1)), static_cast<uint32_t>(AppData.Version.GetIndex(2)));

    DataManagerNamespace::enginedata EngineData = DataManager::GetDataManager().GetEngineData();
    hold.pEngineName = EngineData.Name.c_str();
    hold.engineVersion = VK_MAKE_VERSION(static_cast<uint32_t>(EngineData.Version.GetIndex(0)), static_cast<uint32_t>(EngineData.Version.GetIndex(1)), static_cast<uint32_t>(EngineData.Version.GetIndex(2)));

    hold.apiVersion = VK_API_VERSION_1_0;

    return hold;
}