#ifndef ALCENGINE_MODULE_VULKANHANDLER_PRIVATE_H
#define ALCENGINE_MODULE_VULKANHANDLER_PRIVATE_H

#include <vulkan/vulkan.h>

#include "module/VulkanHandler/VulkanStructBundles.h"

class VulkanHandlerIMPL {
    public:
        VulkanHandlerIMPL();
        ~VulkanHandlerIMPL();

    private:
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR Surface;
        VkDevice Device; //Logical dev, not physical
        struct AlcQueueList {
            VkQueue GraphicsQueue;
            VkQueue SurfacePresentQueue;
        };
        AlcQueueList Q_List;

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

        int CreateSurface();

        int CreateLogicalDevice();
            VkPhysicalDevice SelectPhysicalDevice();
            int ScoreDevice(VkPhysicalDevice _PhysicalDevice);

            void FetchDeviceInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueArray(VkPhysicalDevice _PhysicalDevice, std::vector<VkDeviceQueueCreateInfo>& ReturnArray);

};

#endif