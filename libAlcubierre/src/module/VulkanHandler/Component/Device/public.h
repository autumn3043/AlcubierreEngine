#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_PUBLIC_H

class VulkanDeviceComponent {
    public:
        VulkanDeviceComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanDeviceComponent();
        
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        
        struct DeviceQueue {
            VkQueue queue;
            uint32_t familyIndex;
            uint32_t queueIndex = 0;

            int priority;
            VkQueueFlags supportedQueueTypes;
            bool surfaceSupport;

            DeviceQueue(VkQueueFlags _supportedQueueTypes, int _familyIndex, bool _surfaceSupport = false, int _priority = 1) : supportedQueueTypes(_supportedQueueTypes), familyIndex(static_cast<uint32_t>(_familyIndex)), surfaceSupport(_surfaceSupport), priority(_priority) {};
        };

        DeviceQueue& getQueue(VkQueueFlags type, bool surfaceSupport = false);

    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

        int CreateLogicalDevice();
            VkPhysicalDevice SelectPhysicalDevice();
            int ScoreDevice(VkPhysicalDevice _PhysicalDevice);

            void FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray);
                VkQueueFlags ConfigParse_QueueFlagBits(std::vector<std::string>& values);
                std::vector<DeviceQueue> queues;
            void FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray);
            void FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle);
};

#endif