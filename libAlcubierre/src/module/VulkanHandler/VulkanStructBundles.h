#ifndef VULKAN_STRUCT_BUNDLES_ALC_H
#define VULKAN_STRUCT_BUNDLES_ALC_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <cstring>

std::vector<const char*> c_str_array(const std::vector<std::string>& strings) {
    std::vector<const char*> result;
    result.reserve(strings.size());
    for (const std::string& s : strings) {
        result.push_back(s.c_str());
    }
    return result;
}

struct AlcApplicationInfo {
    public:
        std::string _pApplicationName; //const char*
        int _applicationVersion;
        std::string _pEngineName; //const char*
        int _engineVersion;
        int _apiVersion;

        VkApplicationInfo* Get() {
            InternalStruct = {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pNext = nullptr,
                .pApplicationName = _pApplicationName.c_str(),
                .applicationVersion = static_cast<uint32_t>(_applicationVersion),
                .pEngineName = _pEngineName.c_str(),
                .engineVersion = static_cast<uint32_t>(_engineVersion),
                .apiVersion = static_cast<uint32_t>(_apiVersion)
            };

            return &InternalStruct;
        }
    
    private:
        VkApplicationInfo InternalStruct;
};

struct AlcDebugUtilsMessengerCreateInfoEXT {
    public:
        VkDebugUtilsMessengerCreateFlagsEXT _flags;
        VkDebugUtilsMessageSeverityFlagsEXT _messageSeverity;
        VkDebugUtilsMessageTypeFlagsEXT _messageType;
        PFN_vkDebugUtilsMessengerCallbackEXT _pfnUserCallback;
        void* _pUserData;

        VkDebugUtilsMessengerCreateInfoEXT* Get() {
            InternalStruct = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = _flags,
                .messageSeverity = _messageSeverity,
                .messageType = _messageType,
                .pfnUserCallback = _pfnUserCallback,
                .pUserData = _pUserData
            };

            return &InternalStruct;
        }
    
    private:
        VkDebugUtilsMessengerCreateInfoEXT InternalStruct;

};

struct AlcInstanceCreateInfo {
    public:
        AlcDebugUtilsMessengerCreateInfoEXT _pNext;
        VkInstanceCreateFlags _flags;
        AlcApplicationInfo _pApplicationInfo; //const VkApplicationInfo*
        std::vector<std::string> _ppEnabledLayerNames; //const char* const*
        std::vector<std::string> _ppEnabledExtensionNames; //const char* const*

        VkInstanceCreateInfo* Get() {
            _ppEnabledLayerNames_C = c_str_array(_ppEnabledLayerNames);
            _ppEnabledExtensionNames_C = c_str_array(_ppEnabledExtensionNames);

            InternalStruct = {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = _pNext.Get(),
                .flags = _flags,
                .pApplicationInfo = _pApplicationInfo.Get(),
                .enabledLayerCount = static_cast<uint32_t>(_ppEnabledLayerNames_C.size()),
                .ppEnabledLayerNames = _ppEnabledLayerNames_C.data(),
                .enabledExtensionCount = static_cast<uint32_t>(_ppEnabledExtensionNames_C.size()),
                .ppEnabledExtensionNames = _ppEnabledExtensionNames_C.data()
            };

            return &InternalStruct;
        }

    private:
        std::vector<const char*> _ppEnabledLayerNames_C;
        std::vector<const char*> _ppEnabledExtensionNames_C;
        VkInstanceCreateInfo InternalStruct;
};

struct AlcDeviceFeatures {
    public:
        VkPhysicalDeviceVulkan13Features featuresVk1_3 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = nullptr
        };

        VkPhysicalDeviceVulkan11Features featuresVk1_1 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &featuresVk1_3
        };

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT featuresDynamicState{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
            .pNext = &featuresVk1_1
        };

        VkPhysicalDeviceFeatures2 featuresBase2 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &featuresDynamicState
        };
};

struct AlcDeviceQueueCreateInfo {
    public:
        uint32_t _queueFamilyIndex;
        uint32_t _queueCount;
        float _pQueuePriorities;

        VkDeviceQueueCreateInfo* Get() {
            InternalStruct = VkDeviceQueueCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .queueFamilyIndex = _queueFamilyIndex,
                .queueCount = _queueCount,
                .pQueuePriorities = &_pQueuePriorities
            };

            return &InternalStruct;
        }

    private:
        VkDeviceQueueCreateInfo InternalStruct;
};

struct AlcDeviceCreateInfo {
    public:
        VkDeviceCreateFlags _flags;
        std::vector<AlcDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<VkDeviceQueueCreateInfo> _pQueueCreateInfos;
        std::vector<std::string> _ppEnabledExtensionNames; //const char* const*
        AlcDeviceFeatures _features;

        VkDeviceCreateInfo* Get() {
            _ppEnabledExtensionNames_C = c_str_array(_ppEnabledExtensionNames);

            _pQueueCreateInfos.clear();
            _pQueueCreateInfos.reserve(queueCreateInfos.size());

            for(AlcDeviceQueueCreateInfo& info : queueCreateInfos) {
                _pQueueCreateInfos.push_back(*info.Get());
            }

            InternalStruct = VkDeviceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &_features.featuresBase2,
                .flags = _flags,
                .queueCreateInfoCount = static_cast<uint32_t>(_pQueueCreateInfos.size()),
                .pQueueCreateInfos = _pQueueCreateInfos.data(),
                .enabledExtensionCount = static_cast<uint32_t>(_ppEnabledExtensionNames_C.size()),
                .ppEnabledExtensionNames = _ppEnabledExtensionNames_C.data(),
                .pEnabledFeatures = VK_NULL_HANDLE
            };

            return &InternalStruct;
        }
    
    private:
        std::vector<const char*> _ppEnabledExtensionNames_C;
        VkDeviceCreateInfo InternalStruct;
};

struct AlcImageLayoutDetails {
    VkPipelineStageFlags2 stageMask;
    VkAccessFlags2 accessMask;
    VkImageLayout layout;
    uint32_t queueIndex;

    AlcImageLayoutDetails(VkPipelineStageFlags2 _stageMask, VkAccessFlags2 _accessMask, VkImageLayout _layout, uint32_t _queueIndex = VK_QUEUE_FAMILY_IGNORED)
        :   stageMask(_stageMask),
            accessMask(_accessMask),
            layout(_layout),
            queueIndex(_queueIndex)
        {}

    AlcImageLayoutDetails() {};
};

#endif