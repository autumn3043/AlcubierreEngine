#ifndef VULKAN_STRUCT_BUNDLES_ALC_H
#define VULKAN_STRUCT_BUNDLES_ALC_H

#include <vulkan/vulkan.h>
#include <cstdint>

class AlcDeviceCreateInfo {
    public:
        AlcDeviceCreateInfo() {};
        AlcDeviceCreateInfo(VkDeviceCreateInfo _DeviceCreateInfo) {
            Set(_DeviceCreateInfo);
        }

        void Set(VkDeviceCreateInfo _DeviceCreateInfo) {
            DeviceCreateInfo = _DeviceCreateInfo;

            QueueCreateInfos_ = *_DeviceCreateInfo.pQueueCreateInfos;
            DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfos_;

            EnabledFeatures_ = *_DeviceCreateInfo.pEnabledFeatures;
            DeviceCreateInfo.pEnabledFeatures = &EnabledFeatures_;
        }

        const VkDeviceCreateInfo* Get() const {
            return &DeviceCreateInfo;
        }
    
    private:
        VkDeviceCreateInfo DeviceCreateInfo;

        VkDeviceQueueCreateInfo QueueCreateInfos_;
        VkPhysicalDeviceFeatures EnabledFeatures_;

};

class AlcDeviceQueueCreateInfo {
    public:
        AlcDeviceQueueCreateInfo() {};
        AlcDeviceQueueCreateInfo(VkDeviceQueueCreateInfo _DeviceQueueCreateInfo) {
            Set(DeviceQueueCreateInfo);
        }

        void Set(VkDeviceQueueCreateInfo _DeviceQueueCreateInfo) {
            DeviceQueueCreateInfo = _DeviceQueueCreateInfo;

            QueuePriorities_ = *_DeviceQueueCreateInfo.pQueuePriorities;
            DeviceQueueCreateInfo.pQueuePriorities = &QueuePriorities_;
        }

        const VkDeviceQueueCreateInfo* Get() const {
            return &DeviceQueueCreateInfo;
        }
    
    private:
        VkDeviceQueueCreateInfo DeviceQueueCreateInfo;

        float QueuePriorities_;

};

#endif