#ifndef VULKANHANDLER_ALE_H
#define VULKANHANDLER_ALE_H

#include <vulkan/vulkan.h>

#include "core/Registry/public.h"

//Services

#include <vector>
#include <cstdlib>

#include "module/VulkanHandler/VulkanStructBundles.h"

#include "core/DebugManager/public.h"

class VulkanException : public AlcEngineException {
    public:
        VulkanException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class VulkanHandler {
    public:
        VulkanHandler();
        ~VulkanHandler();

    private:
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkDevice Device;
        VkQueue Queue;

        int Init();

        int CreateVulkanInstance();
            void FetchCreateData(AlcInstanceCreateInfo& ReturnBundle);
            void FetchExtensionData(AlcEnabledExtensions& ReturnBundle);
            void FetchAppData(AlcApplicationInfo& ReturnBundle);

        int CreateDebugLink();
            void FetchDebugData(AlcDebugUtilsMessengerCreateInfoEXT& ReturnBundle);

            static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

        int CreateLogicalDevice(int);
            std::vector<std::pair<int, VkPhysicalDevice>> SelectPhysicalDevice();
            int ScoreDevice(VkPhysicalDevice _PhysicalDevice);

            void FetchDeviceInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceQueueCreateInfo& ReturnBundle);

            std::vector<uint32_t> GetDeviceIndices(VkPhysicalDevice _PhysicalDevice);

};

class VulkanHandlerWrapper : public WrapperBaseClass{
    public:
        VulkanHandlerWrapper() {
            native = new VulkanHandler();
            // Registry::GetRegistry().RegisterService(native->IWindowManager_VulkanHandler);
        }

        ~VulkanHandlerWrapper() {
            delete native;
        }

    private:
        VulkanHandler* native = nullptr;

        static ModuleRegistryBundle bundle;
};

#endif 