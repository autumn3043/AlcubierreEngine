#ifndef VULKANHANDLER_ALE_H
#define VULKANHANDLER_ALE_H

#include <vulkan/vulkan.h>

#include <vector>
#include <cstdlib>

#include "module/VulkanHandler/VulkanStructBundles.h"

class VulkanHandler {
    public:
        VulkanHandler();
        ~VulkanHandler();

    private:
        VkInstance Instance;
        VkDevice Device;
        VkQueue Queue;

        int Init();

        int CreateVulkanInstance();
            void FetchCreateData(AlcInstanceCreateInfo& ReturnBundle);
            void FetchAppData(AlcApplicationInfo& ReturnBundle);

        int CreateLogicalDevice(int);
            std::vector<std::pair<int, VkPhysicalDevice>> SelectPhysicalDevice();
            int ScoreDevice(VkPhysicalDevice _PhysicalDevice);

            void FetchDeviceInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueInfo(VkPhysicalDevice _PhysicalDevice, AlcDeviceQueueCreateInfo& ReturnBundle);

            std::vector<uint32_t> GetDeviceIndices(VkPhysicalDevice _PhysicalDevice);

};

#endif 