#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_DEVICE_PUBLIC_H

class VulkanDeviceComponent {
    public:
        VulkanDeviceComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanDeviceComponent();

    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;
    
    public:
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;

    private:
        struct queueFamily {
            uint32_t deviceFamilyIndex;
            std::vector<VkQueue> queues;

            queueFamily(uint32_t& _deviceFamilyIndex, uint32_t& queueCount) : deviceFamilyIndex(_deviceFamilyIndex) { queues.resize(queueCount); }
        };

        std::vector<queueFamily> queueFamilies;

        std::vector<int> graphicsFamilyIndices;
        std::vector<int> presentFamilyIndices;
        std::vector<int> transferFamilyIndices;

    public:
        enum queueType {
            GRAPHICS = 0,
            PRESENT = 1,
            TRANSFER = 2
        };

        class QueueHandle {
            private:
                queueFamily* family = nullptr;

            public:
                QueueHandle(VulkanDeviceComponent* parent, queueType& type);
                QueueHandle() = default;
                ~QueueHandle();
                QueueHandle(const QueueHandle&) = delete;
                QueueHandle& operator=(const QueueHandle&) = delete;
                QueueHandle(QueueHandle&& other) { *this = std::move(other); }
                QueueHandle& operator=(QueueHandle&& other) {
                    family = other.family;
                    queue = other.queue;
                    deviceQueueFamilyIndex = other.deviceQueueFamilyIndex;

                    other.family = nullptr;
                    other.queue = VK_NULL_HANDLE;
                    
                    return *this;
                }

                uint32_t deviceQueueFamilyIndex;
                VkQueue queue = VK_NULL_HANDLE;
        };

        QueueHandle fetchQueueHandle(queueType type);

    public:
        struct PhysicalDeviceProperties {
            PhysicalDeviceProperties() = default;
            PhysicalDeviceProperties(VulkanHandler* parent, const VkPhysicalDevice& physicalDevice);

            PhysicalDeviceProperties(const PhysicalDeviceProperties& other) = delete;
            PhysicalDeviceProperties& operator=(const PhysicalDeviceProperties& other) = delete;
            PhysicalDeviceProperties& operator=(PhysicalDeviceProperties&& other) = default;

            struct General {
                VkPhysicalDeviceProperties rawStruct;
                bool isDiscrete;
            };

            General general;

            struct Memory {
                VkPhysicalDeviceMemoryProperties rawStruct;
                int hostVisibleIndex;
                int deviceLocalIndex;
            };

            Memory memory;

            struct QueueFamily {
                VkQueueFamilyProperties rawStruct;
                bool hasSurfaceSupport;
            };

            std::vector<QueueFamily> queueFamilies;
        };

        const PhysicalDeviceProperties& fetchDeviceProperties();

    private:
        PhysicalDeviceProperties deviceProperties;
    
    private:
        int CreateLogicalDevice();
            VkPhysicalDevice SelectPhysicalDevice();
            int ScoreDevice(PhysicalDeviceProperties& properties);

            void FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle);
            void FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray);
            void FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray);
            void FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle);
};

#endif