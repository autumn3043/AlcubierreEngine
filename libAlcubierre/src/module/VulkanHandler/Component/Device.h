#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_H

class VulkanDeviceComponent {
    private:
        struct AlcQueue {
            VkQueue queue;
            uint32_t index;
        };

    public:
        VulkanDeviceComponent(VulkanHandlerIMPL* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
            CreateLogicalDevice();
        };

        ~VulkanDeviceComponent();

        VkDevice& getDevice() { return Device; }
        VkPhysicalDevice& getPhysicalDevice() { return PhysicalDevice; }
        AlcQueue& getGraphicsQueue() { return GraphicsQueue; }
        AlcQueue& getSurfacePresentQueue() { return SurfacePresentQueue; }

    private:
        VulkanHandlerIMPL* parent = nullptr;
        Registry*& registry_ptr;
        
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        AlcQueue GraphicsQueue;
        AlcQueue SurfacePresentQueue;

        int CreateLogicalDevice();
            VkPhysicalDevice SelectPhysicalDevice();
            int ScoreDevice(VkPhysicalDevice _PhysicalDevice);

            void FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray);
            void FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray);
            void FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle);
};

#endif