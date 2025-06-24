#ifndef VULKANHANDLER_ALE_H
#define VULKANHANDLER_ALE_H

#include <vulkan/vulkan.h>

class VulkanHandler {
    public:
        VkInstance Instance;
        VkDevice Device;
        VkQueue Queue;

        VulkanHandler();
        ~VulkanHandler();

    private:
        VkApplicationInfo FetchAppData();
        VkInstanceCreateInfo FetchCreateData();

        VkPhysicalDevice PhysicalDevice;

        int CreateVulkanInstance();
        // VkPhysicalDevice SelectPhysicalDevice();
        // VkDevice GetLogicalDevice(VkPhysicalDevice SelectedPhysicalDevice);
        // VkQueue GetDeviceQueue();
};

#endif 