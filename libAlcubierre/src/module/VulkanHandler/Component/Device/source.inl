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

    for(int i = 0; i < queueFamilies.size(); i++) {
        for(int j = 0; j < queueFamilies[i].queues.size(); j++) {
            vkGetDeviceQueue(Device, queueFamilies[i].deviceFamilyIndex, static_cast<uint32_t>(j), &queueFamilies[i].queues[j]);
        }
    }

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

    int best = 0;
    VkPhysicalDevice holdDevice;
    for(const VkPhysicalDevice& device : CompatibleDevices) {
        PhysicalDeviceProperties properties(parent, device);
        int score = ScoreDevice(properties);
        if(score > best) {
            best = score;
            holdDevice = device;
            deviceProperties = std::move(properties);
        }
    }

    return holdDevice;
}

int VulkanDeviceComponent::ScoreDevice(PhysicalDeviceProperties& properties) {
    int hold = 0;

    //Required and preferred properties go here. Required = return 0 if not present. Preferred = add to score.
    if(properties.general.isDiscrete) hold += 10000;
    hold += properties.general.rawStruct.apiVersion;

    //We need to confirm, firstly, that a device *has* all the features we need to begin with. If it doesn't exclude it. If it does we can score it based on the quantity of supporting queues.
    bool _hasGraphics = false;
    bool _hasSurfaceSupport = false;

    for(int i = 0; i < properties.queueFamilies.size(); i++) {
        //TODO: Add logic to prefer GPUs that reflect or better reflect queue requests.
        if(properties.queueFamilies[i].rawStruct.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            hold += 100 * properties.queueFamilies[i].rawStruct.queueCount;
            _hasGraphics = true;
        }

        if(properties.queueFamilies[i].hasSurfaceSupport) _hasSurfaceSupport = true;
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

std::string vectorToString(std::vector<int>& vector) {
    std::string hold;
    for(int i = 0; i < vector.size(); i++) {
        hold += std::to_string(vector[i]);
        hold += " ";
    }

    return hold;
}

void VulkanDeviceComponent::FetchQueueArray(std::vector<AlcDeviceQueueCreateInfo>& ReturnArray) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    for(int i = 0; i < deviceProperties.queueFamilies.size(); i++) {
        uint32_t deviceFamilyIndex = static_cast<uint32_t>(i);
        uint32_t familyQueueCount = deviceProperties.queueFamilies[i].rawStruct.queueCount;

        AlcDeviceQueueCreateInfo& thisQueue = ReturnArray.emplace_back();
        thisQueue._queueCount = familyQueueCount;
        thisQueue._queueFamilyIndex = deviceFamilyIndex;
        thisQueue._pQueuePriorities = 1;

        queueFamily& family = queueFamilies.emplace_back(deviceFamilyIndex, familyQueueCount);
        family.queues.resize(familyQueueCount);

        if(deviceProperties.queueFamilies[i].rawStruct.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamilyIndices.push_back(i);
        if(deviceProperties.queueFamilies[i].hasSurfaceSupport) presentFamilyIndices.push_back(i);
        if(deviceProperties.queueFamilies[i].rawStruct.queueFlags & VK_QUEUE_TRANSFER_BIT) transferFamilyIndices.push_back(i);
    }

    logIdentity("Created " + std::to_string(deviceProperties.queueFamilies.size()) + " queue families." + 
        "\n   Graphics: " + vectorToString(graphicsFamilyIndices) + 
        "\n   Surface: " + vectorToString(presentFamilyIndices) +
        "\n   Transfer: " + vectorToString(transferFamilyIndices)
    );
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
    ReturnBundle.featuresVk1_2.timelineSemaphore = VK_TRUE;
}

VulkanDeviceComponent::QueueHandle::QueueHandle(VulkanDeviceComponent* parent, queueType& type) {
    std::vector<int>* queueFamilyGroup;

    switch(type) {
        case GRAPHICS:
            queueFamilyGroup = &parent->graphicsFamilyIndices;
        break;

        case PRESENT:
            queueFamilyGroup = &parent->presentFamilyIndices;
        break;

        case TRANSFER:
            queueFamilyGroup = &parent->transferFamilyIndices;
        break;
    }

    for(int i = 0; i < queueFamilyGroup->size(); i++) {
        int familyIndex = (*queueFamilyGroup)[i];
        queueFamily* _family = &parent->queueFamilies[familyIndex];
        if(_family->queues.size() == 0) continue;
        family = _family;
        break;
    }

    if(!family) throw VulkanException("Unable to find a free queue within a queue family that supported the requested work type");

    deviceQueueFamilyIndex = family->deviceFamilyIndex;
    queue = family->queues.back();
    family->queues.pop_back();
}

VulkanDeviceComponent::QueueHandle::~QueueHandle() {
    if(family) family->queues.emplace_back(queue);
}

VulkanDeviceComponent::QueueHandle VulkanDeviceComponent::fetchQueueHandle(queueType type) {
    return QueueHandle(this, type);
}

VulkanDeviceComponent::PhysicalDeviceProperties::PhysicalDeviceProperties(VulkanHandler* parent, const VkPhysicalDevice& physicalDevice) {
    vkGetPhysicalDeviceProperties(physicalDevice, &general.rawStruct);
    general.isDiscrete = (general.rawStruct.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memory.rawStruct);
    VkMemoryPropertyFlags deviceLocalMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryPropertyFlags hostVisibleMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    memory.deviceLocalIndex = -1;
    memory.hostVisibleIndex = -1;
    for(int i = 0; i < memory.rawStruct.memoryTypeCount; i++) {
        if((memory.rawStruct.memoryTypes[i].propertyFlags & deviceLocalMemoryProperties) == deviceLocalMemoryProperties && memory.deviceLocalIndex == -1) memory.deviceLocalIndex = i;
        if((memory.rawStruct.memoryTypes[i].propertyFlags & hostVisibleMemoryProperties) == hostVisibleMemoryProperties && memory.hostVisibleIndex == -1) memory.hostVisibleIndex = i;
        if(memory.deviceLocalIndex >= 0 && memory.hostVisibleIndex >= 0) break;
    }

    uint32_t queueFamiliesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, deviceQueueFamilies.data());
    queueFamilies.resize(queueFamiliesCount);
    for(int i = 0; i < queueFamilies.size(); i++) {
        queueFamilies[i].rawStruct = deviceQueueFamilies[i];
        VkBool32 _surfaceSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, parent->environment->Surface, &_surfaceSupport);
        queueFamilies[i].hasSurfaceSupport = _surfaceSupport;
    }
}

VulkanDeviceComponent::PhysicalDeviceProperties& VulkanDeviceComponent::fetchDeviceProperties() {
    return deviceProperties;
}