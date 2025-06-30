#ifndef VULKAN_STRUCT_BUNDLES_ALC_H
#define VULKAN_STRUCT_BUNDLES_ALC_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <cstring>

class AlcInstanceCreateInfo {
    public:
        AlcInstanceCreateInfo() {};
        AlcInstanceCreateInfo(VkInstanceCreateInfo _InstanceCreateInfo) {
            Set(_InstanceCreateInfo);
        }

        void Set(VkInstanceCreateInfo _InstanceCreateInfo) {
            InstanceCreateInfo = _InstanceCreateInfo;

            pApplicationInfo_ = *_InstanceCreateInfo.pApplicationInfo;
            InstanceCreateInfo.pApplicationInfo = &pApplicationInfo_;
        }

        const VkInstanceCreateInfo* Get() const {
            return &InstanceCreateInfo;
        }
    
    private:
        VkInstanceCreateInfo InstanceCreateInfo;

        VkApplicationInfo pApplicationInfo_;
};

class AlcApplicationInfo {
    public:
        AlcApplicationInfo() {
            isSet = false;
        }

        AlcApplicationInfo(VkApplicationInfo _ApplicationInfo) {
            isSet = false;
            Set(_ApplicationInfo);
        }

        void Set(VkApplicationInfo _ApplicationInfo) {
            if(isSet) throw std::logic_error("Cannot modify VkApplicationInfo. Create a new one."); //Because strings are manually freed, setting new value without freeing cause undef behaviour.
            
            isSet = true;

            ApplicationInfo = _ApplicationInfo;

            pApplicationName_ = strdup(_ApplicationInfo.pApplicationName);
            ApplicationInfo.pApplicationName = pApplicationName_;

            pEngineName_ = strdup(_ApplicationInfo.pEngineName);
            _ApplicationInfo.pEngineName = pEngineName_;
        }

        const VkApplicationInfo* Get() const {
            return &ApplicationInfo;
        }

        ~AlcApplicationInfo() {
            free(pApplicationName_);
            free(pEngineName_);
        }
    
    private:
        VkApplicationInfo ApplicationInfo;

        bool isSet;

        char* pApplicationName_;
        char* pEngineName_;
};

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