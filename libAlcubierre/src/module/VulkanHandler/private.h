#ifndef ALCENGINE_MODULE_VULKANHANDLER_PRIVATE_H
#define ALCENGINE_MODULE_VULKANHANDLER_PRIVATE_H

#include <vulkan/vulkan.h>

#include "module/VulkanHandler/VulkanStructBundles.h"

#include <unordered_map>

class VulkanHandlerIMPL {
    public:
        VulkanHandlerIMPL(Registry* registry);
        ~VulkanHandlerIMPL();

    private:
        Registry* registry_ptr = nullptr;

        VkInstance Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR Surface = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        struct AlcQueueList {
            VkQueue GraphicsQueue;
            VkQueue SurfacePresentQueue;
        };
        AlcQueueList Q_List;
        VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
            std::vector<VkImage> ChainImages;
            std::vector<VkImageView> ChainImageViews;
            VkFormat ChainFormat;
            VkExtent2D ChainExtent;
        VkPipeline Pipeline = VK_NULL_HANDLE;

        int Init();

        int CreateVulkanInstance();
            void FetchCreateData(AlcInstanceCreateInfo& ReturnBundle);
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
            void FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray);
            void FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray);
            void FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle);

        int CreateSwapChain();
            int FillSwapchainImages(VkSwapchainCreateInfoKHR& ChainInitInfo);
            void FetchSwapMode(VkPresentModeKHR& ReturnMode);
            void FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor);

        int CreateGraphicsPipeline();
            void FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule);

};

#endif