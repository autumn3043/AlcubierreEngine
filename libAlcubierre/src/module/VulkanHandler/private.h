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
        VkPhysicalDevice PhysicalDevice;
        VkDevice Device;
        struct AlcQueueList {
            VkQueue GraphicsQueue;
            VkQueue SurfacePresentQueue;
        };
        AlcQueueList Q_List;
        VkSwapchainKHR Swapchain;

        int Init();

        int CreateVulkanInstance();
            void FetchCreateData(AlcInstanceCreateInfo& ReturnBundle);
            void FetchInstanceExtensionData(AlcEnabledExtensions& ReturnBundle);
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

            void FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle);
            void FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray);
            void FetchQueueArray(std::vector<VkDeviceQueueCreateInfo>& ReturnArray);

        int CreateSwapChain();
            void FetchSwapMode(VkPresentModeKHR& ReturnMode);
            void FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor);

};

#endif