int VulkanMemoryAllocatorComponent::initTextureHandler() {
    createTextureImage(DEFAULT_TEXTURE_HASH_WHITE, {255, 255, 255, 255}); //White
    createTextureImage(DEFAULT_TEXTURE_HASH_BLACK, {0, 0, 0, 255}); //Black
    createTextureImage(DEFAULT_TEXTURE_HASH_RED, {255, 0, 0, 255}); //Red
    createTextureImage(DEFAULT_TEXTURE_HASH_GREEN, {0, 255, 0, 255}); //Green
    createTextureImage(DEFAULT_TEXTURE_HASH_BLUE, {0, 0, 255, 255}); //Blue

    return 0;
}

//Texture is image and imageview, handle is just imageview
textureImageView* VulkanMemoryAllocatorComponent::getTextureImageView(Hash_T hash) {
    if(!textureImageViews.contains(hash)) {
        createTextureImageView(hash);
    }

    return textureImageViews[hash];
}

int VulkanMemoryAllocatorComponent::createTextureImageView(Hash_t hash) {
    TextureImageView*& textureView = textureImageViews.emplace(hash, new TextureImageView()).first->second;

    if(!textureImages.contains(hash)) hash = DEFAULT_TEXTURE_HASH_RED;

    textureView->pointer = textureImages[hash];

    return 0;
}

int VulkanMemoryAllocatorComponent::createTextureImage(Hash_T hash, std::vector<uint8_t> texels) {
    //TODO: Texels should be a 2/3d array wrapped in a struct that can indicate its width and height so that it can be copied properly
    //copying will require a loop over rows

    VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB

    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr
        .imageType = VK_IMAGE_TYPE_2D,
        .format = textureFormat,
        .extent = {1, 1, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    };
    TextureImage*& texture = textureImages.emplace(hash, new TextureImage()).first->second;
    vkCreateImage(device, &imageCreateInfo, nullptr, &texture->image);

    memoryBlock& stagingMemory = stagingBuffer->acquireMemory(sizeof(uint8_t) * texels.size());
    memcpy(hostVisibleMemoryAccessBaseIndex + stagingMemory.offset, texels.data(), sizeof(uint8_t) * texels.size());

    memoryValidAfter = worker->addTransferOperationToQueue(transferOperation(
        TRANSFER_OPERATION_TYPE_IMAGE,
        transferOperation::region(&stagingBuffer.handle(), stagingMemory.offset, stagingMemory.size), 
        transferOperation::region(&texture->image, 0, stagingMemory.size)
    ));

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = texture->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = textureFormat,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
    };
    vkCreateImageView(device, &imageViewCreateInfo, nullptr, &texture->imageView);

    VkSamplerCreateInfo samplerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_FALSE
    };

    if(textureImageViews.contains(hash)) {
        textureImageViews[hash].pointer = texture;
    } else {
        createTextureImageView(hash);
    }

    return 0;
}

int VulkanMemoryAllocatorComponent::storeTexture(Hash_T hash, std::vector<uint8_t> texels) {
    if(!textureImages.contains(hash)) return createTextureImage(hash, texels);
    else return 1;
}