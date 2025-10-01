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
        VkPhysicalDeviceDynamicRenderingFeatures featuresDynamicRendering {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = VK_NULL_HANDLE
        };

        VkPhysicalDeviceVulkan11Features featuresVk1_1 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &featuresDynamicRendering
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

struct AlcDeviceCreateInfo {
    public:
        VkDeviceCreateFlags _flags;
        std::vector<VkDeviceQueueCreateInfo> _pQueueCreateInfos;
        std::vector<std::string> _ppEnabledExtensionNames; //const char* const*
        AlcDeviceFeatures _features;

        VkDeviceCreateInfo* Get() {
            _ppEnabledExtensionNames_C = c_str_array(_ppEnabledExtensionNames);

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

struct AlcPipelineShaderStageCreateInfo {
    public:
        VkPipelineShaderStageCreateFlags _flags;
        VkShaderStageFlagBits _stage;
        VkShaderModule _module;
        std::string _pName;
        VkSpecializationInfo _pSpecializationInfo;

        VkPipelineShaderStageCreateInfo* Get() {
            InternalStruct = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = _flags,
                .stage = _stage,
                .module = _module,
                .pName = _pName.c_str(),
                .pSpecializationInfo = &_pSpecializationInfo
            };

            return &InternalStruct;
        }

    private:
        VkPipelineShaderStageCreateInfo InternalStruct;
};

#endif