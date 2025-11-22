//Space left for shorthand defines

VulkanSwapchainComponent::VulkanSwapchainComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    CreateSwapchain();
}

VulkanSwapchainComponent::~VulkanSwapchainComponent() {
    for(VkImageView view : ChainImageViews) {
        vkDestroyImageView(parent->device->Device, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    ChainImages.clear();
    vkDestroySwapchainKHR(parent->device->Device, Swapchain, nullptr);
    Swapchain = VK_NULL_HANDLE;
    DM().Log("Successfully destroyed swapchain");
}

int VulkanSwapchainComponent::CreateSwapchain() {
    VkSwapchainCreateInfoKHR ChainInitInfo {};
    ChainInitInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(parent->device->PhysicalDevice, parent->environment->Surface, &surfaceCapabilities);

    IWindowManager::WindowInfo* info = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER))->GetWindowInfo();
    frameWidth = std::clamp<uint32_t>(static_cast<uint32_t>(info->width_pix), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    frameHeight = std::clamp<uint32_t>(static_cast<uint32_t>(info->height_pix), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    frameExtent = { .width=frameWidth, .height=frameHeight };

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
        uint32_t s;
        vkGetSwapchainImagesKHR(parent->device->Device, Swapchain, &s, nullptr);
        DM().Log("Successfully created Vulkan swapchain consisting of " + std::to_string(s) + " images");
        return FillSwapchainImages(ChainInitInfo);
    } else {
        throw VulkanException("Failed to create Vulkan swapchain due to VK error: " + std::to_string(hold));
    }
}

int VulkanSwapchainComponent::FillSwapchainImages(VkSwapchainCreateInfoKHR& ChainInitInfo) {
    ChainFormat = ChainInitInfo.imageFormat;

    uint32_t s;
    vkGetSwapchainImagesKHR(parent->device->Device, Swapchain, &s, nullptr);
    ChainImages.resize(s);
    vkGetSwapchainImagesKHR(parent->device->Device, Swapchain, &s, ChainImages.data());

    VkImageSubresourceRange ImageSubresourceRange {};
    ImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageSubresourceRange.baseMipLevel = 0;
    ImageSubresourceRange.levelCount = 1;
    ImageSubresourceRange.baseArrayLayer = 0;
    ImageSubresourceRange.layerCount = 1;

    int successes = 0;
    for(int i = 0; i < ChainImages.size(); i++) {
        ChainImageViews.push_back(VkImageView());

        VkImageViewCreateInfo ImageViewCreateInfo {};
        ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        ImageViewCreateInfo.image = ChainImages[i];
        ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCreateInfo.format = ChainFormat;
        ImageViewCreateInfo.subresourceRange = ImageSubresourceRange;

        VkResult hold = vkCreateImageView(parent->device->Device, &ImageViewCreateInfo, nullptr, &ChainImageViews[i]);

        if(hold != VK_SUCCESS) DM().Log("Failed to acquire image view on index [" + std::to_string(i) + "]");
        else successes++;
    }

    if(successes > 0) return 0;
    else throw VulkanException("No image views were able to be acquired.");
}

void VulkanSwapchainComponent::FetchSwapMode(VkPresentModeKHR& ReturnMode) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(parent->device->PhysicalDevice, parent->environment->Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(parent->device->PhysicalDevice, parent->environment->Surface, &presentModeCount, presentModes.data());

    std::vector<int> PreferredMode = CM->Get<std::vector<int>>(std::vector<std::string>{"graphics", "presentation_mode"}, std::vector<int>{VK_PRESENT_MODE_FIFO_KHR});
    for(int i = 0; i < PreferredMode.size(); i++) {
        for(const VkPresentModeKHR& mode : presentModes) {
            if(PreferredMode[i] == mode) {
                ReturnMode = mode;
                return;
            }
        }
    }

    throw VulkanException("No available present mode was suitable.");
}

void VulkanSwapchainComponent::FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(parent->device->PhysicalDevice, parent->environment->Surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(parent->device->PhysicalDevice, parent->environment->Surface, &surfaceFormatCount, surfaceFormats.data());

    std::vector<int> PreferredFormat = CM->Get<std::vector<int>>(std::vector<std::string>{"graphics", "image_format"}, std::vector<int>{VK_FORMAT_B8G8R8A8_SRGB});
    for(int i = 0; i < PreferredFormat.size(); i++) {
        for(const VkSurfaceFormatKHR& imageFormat : surfaceFormats) {
            if(PreferredFormat[i] == imageFormat.format) {
                ReturnFormat = imageFormat.format;
                ReturnColor = imageFormat.colorSpace;
                return;
            }
        }
    }

    //Just use any format
    ReturnFormat = surfaceFormats[0].format;
    ReturnColor = surfaceFormats[0].colorSpace;
    DM().Log("No requested image format was available");
}

//Undefine shorthands!!