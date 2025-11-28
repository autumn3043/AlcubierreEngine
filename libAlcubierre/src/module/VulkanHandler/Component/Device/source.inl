//Space left for shorthand defines
#define NULL_BIT 0x0

VulkanDeviceComponent::VulkanDeviceComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    CreateLogicalDevice();
};

VulkanDeviceComponent::~VulkanDeviceComponent() {
    if(Device != VK_NULL_HANDLE) {
        vkDestroyDevice(Device, nullptr);
        Device = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed Vulkan logical device");
    }
}

int VulkanDeviceComponent::CreateLogicalDevice() {
    PhysicalDevice = SelectPhysicalDevice();

    AlcDeviceCreateInfo DeviceInitInfo;
    FetchDeviceInfo(DeviceInitInfo);

    VkResult CreateEcho = vkCreateDevice(PhysicalDevice, DeviceInitInfo.Get(), nullptr, &Device);

    vkGetDeviceQueue(Device, GraphicsQueue.index, 0, &GraphicsQueue.queue);
    vkGetDeviceQueue(Device, SurfacePresentQueue.index, 0, &SurfacePresentQueue.queue);

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
        DM().Log("Successfully instantiated Vulkan logical device");

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
    int RequestedQueues = CM->Get<int>(std::vector<std::string>{"graphics", "device", "queues", "SIZE_T"}, 0);

    if(RequestedQueues == 0) {
        DM().Log("Applying default graphics queue settings", 1);

        //Graphics queue
        CM->Set<std::vector<int>>({"graphics", "device", "queues", "0", "flags"}, "[1]");
        CM->Set<int>({"graphics", "device", "queues", "0", "count"}, "1");
        CM->Set<int>({"graphics", "device", "queues", "0", "priority"}, "1");

        //Surface queue
        CM->Set<bool>({"graphics", "device", "queues", "1", "surface_support"}, "true");
        CM->Set<int>({"graphics", "device", "queues", "1", "count"}, "1");
        CM->Set<int>({"graphics", "device", "queues", "1", "priority"}, "1");

        RequestedQueues = 2;
    }

    std::unordered_map<uint32_t, VkQueueFlags> AvailableQueues;
    
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    std::vector<int> allocatedQueues;
    for (int i = 0; i < RequestedQueues; i++) {

        AlcDeviceQueueCreateInfo& queueCreateInfo = ReturnArray.emplace_back(AlcDeviceQueueCreateInfo());

        //More verbose than strictly necessary to facilitate human readable values in config sheet
        std::vector<std::string> _requiredFlags = CM->Get<std::vector<std::string>>({"graphics", "device", "queues", std::to_string(i), "flags"}, {});
        VkQueueFlags requiredFlags = ConfigParse_QueueFlagBits(_requiredFlags);
        bool SurfaceRequirement = CM->Get<bool>({"graphics", "device", "queues", std::to_string(i), "surface_support"}, false);

        std::vector<int> usableQueues;

        for(int j = 0; j < QueueFamilies.size(); j++) {
            if((QueueFamilies[j].queueFlags & requiredFlags) != requiredFlags) continue;

            if(SurfaceRequirement) {
                VkBool32 SurfaceSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, j, parent->environment->Surface, &SurfaceSupport);
                if(!SurfaceSupport) continue;
            }

            bool skip = false;
            for(int& queue : allocatedQueues) {
                if(j == queue) {
                    skip = true;
                    break;
                }
            }
            if(QueueFamilyCount - allocatedQueues.size() == 0) skip = false;
            if(skip) continue;

            usableQueues.push_back(j);
        }

        if(usableQueues.size() == 0) throw VulkanException("No available graphics queue fulfilled all requirements for queue: " + std::to_string(i));

        queueCreateInfo._queueFamilyIndex = static_cast<uint32_t>(usableQueues[0]);
        if(SurfaceRequirement) {
            SurfacePresentQueue.index = static_cast<uint32_t>(usableQueues[0]);
        } else {
            GraphicsQueue.index = static_cast<uint32_t>(usableQueues[0]);
        }

        queueCreateInfo._queueCount = static_cast<uint32_t>(CM->Get<int>({"graphics", "device", "queues", std::to_string(i), "count"}, 1));
        queueCreateInfo._pQueuePriorities = static_cast<float>(CM->Get<int>({"graphics", "device", "queues", std::to_string(i), "priority"}, 1));
        allocatedQueues.push_back(usableQueues[0]);
    }
}

VkQueueFlags VulkanDeviceComponent::ConfigParse_QueueFlagBits(std::vector<std::string>& values) {
    VkQueueFlags returnValue = NULL_BIT;
    for(std::string& value : values) {
        if(value == "graphics") {
            returnValue |= VK_QUEUE_GRAPHICS_BIT;
            continue;
        }
        if(value == "compute") {
            returnValue |= VK_QUEUE_COMPUTE_BIT;
            continue;
        }
        if(value == "transfer") {
            returnValue |= VK_QUEUE_TRANSFER_BIT;
            continue;
        }
        if(value == "sparse_binding") {
            returnValue |= VK_QUEUE_SPARSE_BINDING_BIT;
            continue;
        }
    }
    return returnValue;
}

void VulkanDeviceComponent::FetchDeviceExtensionArray(std::vector<std::string>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    //Stuff other modules need.
    std::vector<std::string> requestedExtensions = CM->Get<std::vector<std::string>>({"device", "extensions"}, {});

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
#undef NULL_BIT