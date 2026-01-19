//Space left for shorthand defines

VulkanDeviceComponent::VulkanDeviceComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    CreateLogicalDevice();
};

VulkanDeviceComponent::~VulkanDeviceComponent() {
    if(Device != VK_NULL_HANDLE) {
        vkDestroyDevice(Device, nullptr);
        Device = VK_NULL_HANDLE;
        logIdentity("Successfully destroyed Vulkan logical device");
    }
}

int VulkanDeviceComponent::CreateLogicalDevice() {
    PhysicalDevice = SelectPhysicalDevice();

    AlcDeviceCreateInfo DeviceInitInfo;
    FetchDeviceInfo(DeviceInitInfo);

    VkResult CreateEcho = vkCreateDevice(PhysicalDevice, DeviceInitInfo.Get(), nullptr, &Device);

    for(DeviceQueue& queue : queues) vkGetDeviceQueue(Device, queue.familyIndex, queue.queueIndex, &queue.queue);

    if(CreateEcho != VK_SUCCESS) {
        switch(CreateEcho) {
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                throw VulkanException("Requested extension not present in device marked compatible.");
            break;

            case VK_ERROR_FEATURE_NOT_PRESENT:
                throw VulkanException("Requested feature not present in device marked compatible.");
            break;

            default:
                throw VulkanException("Failed to intialise logical device.");
            break;
        }

        return 1;

    } else {
        logIdentity("Successfully instantiated Vulkan logical device");

        return 0;
    }
}

VkPhysicalDevice VulkanDeviceComponent::SelectPhysicalDevice() {
    uint32_t CompatibleDeviceCount = 0;
    vkEnumeratePhysicalDevices(parent->environment->Instance, &CompatibleDeviceCount, nullptr);
    if(CompatibleDeviceCount == 0) throw std::runtime_error("No detected GPU devices support Vulkan.");
    std::vector<VkPhysicalDevice> CompatibleDevices(CompatibleDeviceCount);
    vkEnumeratePhysicalDevices(parent->environment->Instance, &CompatibleDeviceCount, CompatibleDevices.data());

    int u = 0;
    VkPhysicalDevice hold;
    for(const VkPhysicalDevice& device : CompatibleDevices) {
        int score = ScoreDevice(device);
        if(score > u) {
            u = score;
            hold = device;
        }
    }

    return hold;
}

int VulkanDeviceComponent::ScoreDevice(VkPhysicalDevice _PhysicalDevice) {    
    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(_PhysicalDevice, &DeviceProperties);

    int hold = 0;

    //Required and preferred properties go here. Required = return 0 if not present. Preferred = add to score.
    if(DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        hold += 10000;
    }

    hold += DeviceProperties.apiVersion;

    VkPhysicalDeviceFeatures AvailableFeatures;
    vkGetPhysicalDeviceFeatures(_PhysicalDevice, &AvailableFeatures);

    uint32_t DeviceQueueFamiliesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &DeviceQueueFamiliesCount, nullptr);
    std::vector<VkQueueFamilyProperties> DeviceQueueFamilies(DeviceQueueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &DeviceQueueFamiliesCount, DeviceQueueFamilies.data());

    //We need to confirm, firstly, that a device *has* all the features we need to begin with. If it doesn't exclude it. If it does we can score it based on the quantity of supporting queues.
    bool _hasGraphics = false;
    bool _hasSurfaceSupport = false;

    for(int i = 0; i < DeviceQueueFamilies.size(); i++) {
        //TODO: Add logic to prefer GPUs that reflect or better reflect queue requests.
        if(DeviceQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            hold += 100 * DeviceQueueFamilies[i].queueCount;
            _hasGraphics = true;
        }

        VkBool32 SurfaceSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(_PhysicalDevice, i, parent->environment->Surface, &SurfaceSupport);
        if(SurfaceSupport) _hasSurfaceSupport = true;
    }

    if(!(_hasGraphics && _hasSurfaceSupport)) {
        return 0;
    }

    return hold;
}

void VulkanDeviceComponent::FetchDeviceInfo(AlcDeviceCreateInfo& ReturnBundle) {
    ReturnBundle._flags = 0;
    FetchQueueArray(ReturnBundle.queueCreateInfos);
    FetchDeviceExtensionArray(ReturnBundle._ppEnabledExtensionNames);
    FetchDeviceFeatures(ReturnBundle._features);
}

void VulkanDeviceComponent::FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    /*
    int desiredGraphicsQueues = CM->Get<int>({"graphics", "device", "queues", "graphics_capable_count"}, 1);
    int desiredComputeQueues = CM->Get<int>({"graphics", "device", "queues", "compute_capable_count"}, 0);
    int desiredTransferQueues = CM->Get<int>({"graphics", "device", "queues", "transfer_capable_count"}, 1);
    int desiredBindingQueues = CM->Get<int>({"graphics", "device", "queues", "binding_capable_count"}, 0);
    */
    int desiredGraphicsQueues = 1;
    int desiredComputeQueues = 0;
    int desiredTransferQueues = 1;
    int desiredBindingQueues = 0;

    for(int i = 0; i < QueueFamilyCount; i++) {
        if(desiredGraphicsQueues + desiredComputeQueues + desiredTransferQueues + desiredBindingQueues == 0) break;

        VkQueueFlags supportedTypes = NULL_BIT;

        if(QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && desiredGraphicsQueues > 0) {
            supportedTypes |= VK_QUEUE_GRAPHICS_BIT;
            desiredGraphicsQueues--;
        };
        if(QueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && desiredComputeQueues > 0) {
            supportedTypes |= VK_QUEUE_COMPUTE_BIT;
            desiredComputeQueues--;
        };
        if(QueueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && desiredTransferQueues > 0) {
            supportedTypes |= VK_QUEUE_TRANSFER_BIT;
            desiredTransferQueues--;
        };
        if(QueueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT && desiredBindingQueues > 0) {
            supportedTypes |= VK_QUEUE_SPARSE_BINDING_BIT;
            desiredBindingQueues--;
        };

        if(supportedTypes != NULL_BIT) {
            AlcDeviceQueueCreateInfo& thisQueue = ReturnArray.emplace_back();
            thisQueue._queueCount = 1;
            thisQueue._queueFamilyIndex = static_cast<uint32_t>(i);
            thisQueue._pQueuePriorities = 1;

            VkBool32 surfaceSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, parent->environment->Surface, &surfaceSupport);

            queues.emplace_back(supportedTypes, i, surfaceSupport);
        }
    }
}

VulkanDeviceComponent::DeviceQueue& VulkanDeviceComponent::getQueue(VkQueueFlags type, bool surfaceSupport = false) {
    DeviceQueue& hold = queues[0];
    int holdPriority = -1;

    for(DeviceQueue& queue : queues) {
        if(surfaceSupport) {
            if(!queue.surfaceSupport) continue;
        }

        if((type & queue.supportedQueueTypes) == type) {
            if(queue.priority > holdPriority) {
                hold = queue;
                holdPriority = queue.priority;
            }
        }
    }

    if(holdPriority == -1) throw VulkanException("No valid device queue was found");

    return hold;
}

void VulkanDeviceComponent::FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    //Stuff other modules need.
    std::vector<std::string> requestedExtensions = CM->Get<std::vector<std::string>>({"device", "extensions"}, {}, nullptr);

    //Stuff Vulkan needs.
    requestedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    requestedExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    requestedExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);

    //Duplicates are gracefully ignored. Source: I made it the fuck up, shit man I really hope they are.
    for(std::string extension : requestedExtensions) {
        ReturnArray.emplace_back(extension);
    }
}

void VulkanDeviceComponent::FetchDeviceFeatures(AlcDeviceFeatures& ReturnBundle) {
    // ReturnBundle.featuresBase.geometryShader = VK_TRUE;

    ReturnBundle.featuresVk1_3.dynamicRendering = VK_TRUE;
    ReturnBundle.featuresVk1_1.shaderDrawParameters = VK_TRUE;
    ReturnBundle.featuresDynamicState.extendedDynamicState = VK_TRUE;
    ReturnBundle.featuresVk1_3.synchronization2 = VK_TRUE; //For dynamic rendering
}

//Undefine shorthands!!