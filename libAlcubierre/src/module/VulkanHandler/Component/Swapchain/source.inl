//Space left for shorthand defines

VulkanSwapchainComponent::VulkanSwapchainComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    CreateSwapchain();
}

VulkanSwapchainComponent::~VulkanSwapchainComponent() {
    vkDestroySwapchainKHR(parent->device->Device, Swapchain, nullptr);
    Swapchain = VK_NULL_HANDLE;
    logIdentity("Successfully destroyed swapchain");
}

std::vector<AlcImageLayoutDetails> VulkanSwapchainComponent::imageLayoutPresets = {
    {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, {}, VK_IMAGE_LAYOUT_UNDEFINED},
    {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    {VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, {}, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}
};

int VulkanSwapchainComponent::CreateSwapchain() {
    VkSwapchainCreateInfoKHR ChainInitInfo {};
    ChainInitInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(parent->device->PhysicalDevice, parent->environment->Surface, &surfaceCapabilities);

    IWindowManager::WindowInfo info = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER))->getWindowInfo();
    frameWidth = std::clamp<uint32_t>(static_cast<uint32_t>(info.width_pix), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    frameHeight = std::clamp<uint32_t>(static_cast<uint32_t>(info.height_pix), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    frameExtent = { .width=frameWidth, .height=frameHeight };

    // logIdentity(std::to_string(frameWidth) + "w " + std::to_string(frameHeight) + "h");

    ChainInitInfo.imageExtent = frameExtent;
    ChainInitInfo.imageArrayLayers = 1;
    ChainInitInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ChainInitInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    ChainInitInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ChainInitInfo.preTransform = surfaceCapabilities.currentTransform;

    FetchSwapMode(ChainInitInfo.presentMode);
    FetchSwapSurfaceFormat(ChainInitInfo.imageFormat, ChainInitInfo.imageColorSpace);

    ChainInitInfo.surface = parent->environment->Surface;

    VkResult hold = vkCreateSwapchainKHR(parent->device->Device, &ChainInitInfo, nullptr, &Swapchain);

    if(hold == VK_SUCCESS) {
        logIdentity("Successfully created Vulkan swapchain");
        return FillSwapchainImages(ChainInitInfo);
    } else {
        throw VulkanException("Failed to create Vulkan swapchain due to VK error: " + std::to_string(hold));
    }
}

VulkanSwapchainComponent::SwapchainImageWrapper::SwapchainImageWrapper(VulkanSwapchainComponent* _parent, VkDevice& _device, VkExtent2D _extent) : parent(_parent), device(_device), extent(_extent) {
    layoutDetails = imageLayoutPresets[LAYOUT_DETAILS_PRESET_UNDEFINED];

    VkSemaphoreCreateInfo CreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(device, &CreateInfo, nullptr, &semaphore);
}

VulkanSwapchainComponent::SwapchainImageWrapper::~SwapchainImageWrapper() {
    if(imageView) vkDestroyImageView(device, imageView, nullptr);
    imageView = VK_NULL_HANDLE;

    if(semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
}

int VulkanSwapchainComponent::SwapchainImageWrapper::TransitionImageLayout(VkCommandBuffer& CommandBuffer, LAYOUT_DETAILS_PRESET preset) {
    AlcImageLayoutDetails& target = parent->imageLayoutPresets[preset];

    VkImageMemoryBarrier2 imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,

        .srcStageMask = layoutDetails.stageMask,
        .srcAccessMask = layoutDetails.accessMask,
        .dstStageMask = target.stageMask,
        .dstAccessMask = target.accessMask,
        .oldLayout = layoutDetails.layout,
        .newLayout = target.layout,
        .srcQueueFamilyIndex = layoutDetails.queueIndex,
        .dstQueueFamilyIndex = target.queueIndex,

        .image = imageHandle,

        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkDependencyInfo dependencyInfo {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageMemoryBarrier
    };

    vkCmdPipelineBarrier2(CommandBuffer, &dependencyInfo);

    layoutDetails = target;

    return 0;
}

int VulkanSwapchainComponent::FillSwapchainImages(VkSwapchainCreateInfoKHR& ChainInitInfo) {
    ChainFormat = ChainInitInfo.imageFormat;

    uint32_t s;
    Images.clear();
    vkGetSwapchainImagesKHR(parent->device->Device, Swapchain, &s, nullptr);
    std::vector<VkImage> hold(s, VK_NULL_HANDLE);
    VkResult imageCreateResult = vkGetSwapchainImagesKHR(parent->device->Device, Swapchain, &s, hold.data());
    Images.reserve(s);
    for(int i = 0; i < s; i++) {
        SwapchainImageWrapper& image = Images.emplace_back(this, parent->device->Device, frameExtent);
        image.imageHandle = hold[i];
    }
    if(imageCreateResult == VK_SUCCESS) logIdentity("Successfully created " + std::to_string(s) + " swapchain images");
    else throw VulkanException("An issue occurred when attempting to create "+ std::to_string(s) + " swapchain images");

    VkImageSubresourceRange ImageSubresourceRange {};
    ImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageSubresourceRange.baseMipLevel = 0;
    ImageSubresourceRange.levelCount = 1;
    ImageSubresourceRange.baseArrayLayer = 0;
    ImageSubresourceRange.layerCount = 1;

    int successes = 0;
    for(SwapchainImageWrapper& image : Images) {
        VkImageViewCreateInfo ImageViewCreateInfo {};
        ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ImageViewCreateInfo.image = image.imageHandle;
        ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCreateInfo.format = ChainFormat;
        ImageViewCreateInfo.subresourceRange = ImageSubresourceRange;

        VkResult imageViewCreateResult = vkCreateImageView(parent->device->Device, &ImageViewCreateInfo, nullptr, &image.imageView);
        if(imageViewCreateResult == VK_SUCCESS) successes++;
    }

    if(successes == Images.size()) logIdentity("Successfully allocated all image views");
    else if(successes > 0) logIdentity("Failed to allocate " + std::to_string(Images.size() - successes) + " image views", 2);
    else throw VulkanException("Failed to allocate any image views");

    return 0;
}

void VulkanSwapchainComponent::FetchSwapMode(VkPresentModeKHR& ReturnMode) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(parent->device->PhysicalDevice, parent->environment->Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(parent->device->PhysicalDevice, parent->environment->Surface, &presentModeCount, presentModes.data());
    if(presentModeCount == 0) throw VulkanException("The physical device indicated that there are no valid presentation modes");

    std::vector<std::string> PreferredMode_str = CM->Get<std::vector<std::string>>({"graphics", "acceptable_presentation_modes"}, {"vsync"});
    std::vector<int> PreferredMode; 
    for(int i = 0; i < PreferredMode_str.size(); i++) {
        std::string mode = PreferredMode_str[i];
        if(mode == "immediate") {
            PreferredMode.push_back(VK_PRESENT_MODE_IMMEDIATE_KHR);
            continue;
        }
        if(mode == "triple_buffered") {
            PreferredMode.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
            continue;
        }
        if(mode == "vsync") {
            PreferredMode.push_back(VK_PRESENT_MODE_FIFO_KHR);
            continue;
        }
        if(mode == "relaxed_vsync") {
            PreferredMode.push_back(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
            continue;
        }
        logIdentity("Presentation mode preference #" + std::to_string(i) + " '" + mode + "' is not a recognised mode and will be ignored", 1);
        PreferredMode_str.erase(PreferredMode_str.begin() + i);
    }
    for(int i = 0; i < PreferredMode.size(); i++) {
        for(const VkPresentModeKHR& mode : presentModes) {
            if(PreferredMode[i] == mode) {
                ReturnMode = mode;
                logIdentity("Selected presentation mode '" + PreferredMode_str[i] + "' for swapchain");
                return;
            }
        }
        logIdentity("Application preferred presentation mode '" + PreferredMode_str[i] + "' was not available", 1);
    }

    logIdentity("No presentation mode which the application indicated was acceptable was available", 2);
    int engineDefault = CM->Get<int>({"protected", "graphics", "default_presentation_mode"});
    for(const VkPresentModeKHR& mode : presentModes) {
        if(engineDefault == mode) {
            ReturnMode = mode;
            return;
        } else {
            logIdentity("The engine fallback presentation mode was not available", 2);
        }
    }

    logIdentity("Since no listed presentation mode was available, falling back to the first mode indicated by the device", 2);
    ReturnMode = presentModes[0];
}

void VulkanSwapchainComponent::FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(parent->device->PhysicalDevice, parent->environment->Surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(parent->device->PhysicalDevice, parent->environment->Surface, &surfaceFormatCount, surfaceFormats.data());

    std::vector<int> preferredFormat = CM->Get<std::vector<int>>(std::vector<std::string>{"graphics", "acceptable_image_formats"}, std::vector<int>{VK_FORMAT_B8G8R8A8_SRGB});
    for(int i = 0; i < preferredFormat.size(); i++) {
        for(const VkSurfaceFormatKHR& imageFormat : surfaceFormats) {
            if(preferredFormat[i] == imageFormat.format) {
                ReturnFormat = imageFormat.format;
                ReturnColor = imageFormat.colorSpace;
                return;
            }
        }
    }

    //Just use any format
    ReturnFormat = surfaceFormats[0].format;
    ReturnColor = surfaceFormats[0].colorSpace;
    logIdentity("No requested image format was available", 2);
}

//Undefine shorthands!!