#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_PUBLIC_H

class VulkanDeviceComponent {
    private:
        struct AlcQueue {
            VkQueue queue;
            uint32_t index;
        };

    public:
        VulkanDeviceComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanDeviceComponent();
        
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        AlcQueue GraphicsQueue;
        AlcQueue SurfacePresentQueue;

    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

        int CreateLogicalDevice();
            VkPhysicalDevice SelectPhysicalDevice();
            int ScoreDevice(VkPhysicalDevice _PhysicalDevice);

            void FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray);
            void FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray);
            void FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle);
};

#endif