#ifndef VULKAN_STRUCT_BUNDLES_ALC_H
#define VULKAN_STRUCT_BUNDLES_ALC_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <cstring>

class AlcInstanceCreateInfo {
    public:
        AlcInstanceCreateInfo() {};

        void Set(VkInstanceCreateInfo _InstanceCreateInfo) {
            InstanceCreateInfo = _InstanceCreateInfo;

            pApplicationInfo_ = *_InstanceCreateInfo.pApplicationInfo;
            InstanceCreateInfo.pApplicationInfo = &pApplicationInfo_;

            for(int i = 0; i < _InstanceCreateInfo.enabledExtensionCount; i++) {
                EnabledExtensionsStrs_.emplace_back(_InstanceCreateInfo.ppEnabledExtensionNames[i]);
            }
            for(const std::string& str : EnabledExtensionsStrs_) EnabledExtensions_.push_back(str.c_str());
            InstanceCreateInfo.ppEnabledExtensionNames = EnabledExtensions_.data();
            InstanceCreateInfo.enabledExtensionCount = EnabledExtensions_.size();

            for(int i = 0; i < _InstanceCreateInfo.enabledLayerCount; i++) {
                EnabledLayersStrs_.emplace_back(_InstanceCreateInfo.ppEnabledLayerNames[i]);
            }
            for(const std::string& str : EnabledLayersStrs_) EnabledLayers_.push_back(str.c_str());
            InstanceCreateInfo.ppEnabledLayerNames = EnabledLayers_.data();
            InstanceCreateInfo.enabledLayerCount = EnabledLayers_.size();
        }

        VkInstanceCreateInfo* Get() {
            return &InstanceCreateInfo;
        }
    
    private:
        VkInstanceCreateInfo InstanceCreateInfo;

        VkApplicationInfo pApplicationInfo_;

        std::vector<std::string> EnabledExtensionsStrs_;
        std::vector<const char*> EnabledExtensions_;
        std::vector<std::string> EnabledLayersStrs_;
        std::vector<const char*> EnabledLayers_;
};

class AlcEnabledExtensions {
    public:
        AlcEnabledExtensions() {};
        AlcEnabledExtensions(std::vector<std::string> _EnabledExtensions) {
            Set(_EnabledExtensions);
        }

        void Set(std::vector<std::string> _EnabledExtensions) {
            ExtensionNames = std::move(_EnabledExtensions);

            ExtensionNamePointers.clear();
            ExtensionNamePointers.reserve(ExtensionNames.size());

            for(const std::string& extension : ExtensionNames) {
                ExtensionNamePointers.push_back(extension.c_str());
            }
        }

        const std::vector<const char*>* Get() const {
            return &ExtensionNamePointers;
        }

    private:
        std::vector<std::string> ExtensionNames;
        std::vector<const char*> ExtensionNamePointers;
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

            EnabledFeatures_ = *_DeviceCreateInfo.pEnabledFeatures;
            DeviceCreateInfo.pEnabledFeatures = &EnabledFeatures_;

            for(int i = 0; i < _DeviceCreateInfo.enabledExtensionCount; i++) {
                EnabledExtensionsStrs_.emplace_back(_DeviceCreateInfo.ppEnabledExtensionNames[i]);
            }
            for(const std::string& str : EnabledExtensionsStrs_) EnabledExtensions_.push_back(str.c_str());
            DeviceCreateInfo.ppEnabledExtensionNames = EnabledExtensions_.data();
            DeviceCreateInfo.enabledExtensionCount = EnabledExtensions_.size();

            for(int i = 0; i < _DeviceCreateInfo.queueCreateInfoCount; i++) {
                QueueCreateInfos_.emplace_back(_DeviceCreateInfo.pQueueCreateInfos[i]);
            }
            DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos_.data();
            DeviceCreateInfo.queueCreateInfoCount = QueueCreateInfos_.size();
        }

        const VkDeviceCreateInfo* Get() const {
            return &DeviceCreateInfo;
        }
    
    private:
        VkDeviceCreateInfo DeviceCreateInfo;

        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos_;
        VkPhysicalDeviceFeatures EnabledFeatures_;
        std::vector<std::string> EnabledExtensionsStrs_;
        std::vector<const char*> EnabledExtensions_;

};

class AlcDeviceQueueCreateInfo {
    public:
        AlcDeviceQueueCreateInfo() {};
        AlcDeviceQueueCreateInfo(VkDeviceQueueCreateInfo _DeviceQueueCreateInfo) {
            Set(_DeviceQueueCreateInfo);
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

class AlcDebugUtilsMessengerCreateInfoEXT {
    public:
        AlcDebugUtilsMessengerCreateInfoEXT() {};
        AlcDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT _DebugUtilsMessengerCreateInfoEXT) {
            Set(_DebugUtilsMessengerCreateInfoEXT);
        }

        void Set(VkDebugUtilsMessengerCreateInfoEXT _DebugUtilsMessengerCreateInfoEXT) {
            DebugUtilsMessengerCreateInfoEXT = _DebugUtilsMessengerCreateInfoEXT;

            pfnUserCallback_ = _DebugUtilsMessengerCreateInfoEXT.pfnUserCallback;
            DebugUtilsMessengerCreateInfoEXT.pfnUserCallback = pfnUserCallback_;

            DebugUtilsMessengerCreateInfoEXT.pUserData = nullptr;
        }

        const VkDebugUtilsMessengerCreateInfoEXT* Get() const {
            return &DebugUtilsMessengerCreateInfoEXT;
        }
    
    private:
        VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT;

        PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallback_;

};

#endif