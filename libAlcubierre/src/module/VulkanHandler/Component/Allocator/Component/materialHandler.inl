int VulkanMemoryAllocatorComponent::initMaterialHandler() {
    storeMaterial(DEFAULT_MATERIAL_HASH, MaterialTextureHashList());

    return 0;
}

int VulkanMemoryAllocatorComponent::destroyMaterialHandler() {
    return 0;
}

int VulkanMemoryAllocatorComponent::storeMaterial(Hash_T hash, MaterialTextureHashList textureHashes) {
    if(!materials.contains(hash)) {
        materials.emplace(hash, new materialHandle);
        material->textures = textureHashes;
        material->descriptorSetIndex = acquireDescriptorSet();
    }

    materialHandle*& material = materials[hash];

    std::vector<VkWriteDescriptorSet> writeDescriptorSets;

    for(int i = 0; i < textureHashes.size(); i++) {
        Hash_T& textureHash = textureHashes[i];
        if(textureHash == 0) continue;

        VulkanMemoryAllocatorComponent::TextureImageView view = getTextureImageView(textureHash);
        VkDescriptorImageInfo& descriptorImageInfo = {*view->pointer->sampler, *view->pointer->imageView, *view->pointer->imageLayout};
        writeDescriptorSets.emplace_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = fetchDescriptorSet(material->descriptorSetIndex),
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfo
        });
    }

    vkUpdateDescriptorSets(parent->device->Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

    return 0;
}

int VulkanMemoryAllocatorComponent::discardMaterial(Hash_T hash) {
    freeDescriptorSet(materials[hash].descriptorSetIndex);
    materials.erase(hash);

    return 0;
}

int VulkanMemoryAllocatorComponent::incrementMaterialConsumers(Hash_T hash) {
    materials[hash].consumers++;

    return 0;
}

int VulkanMemoryAllocatorComponent::decrementMaterialConsumers(Hash_T hash) {
    materials[hash].consumers--;

    if(materials[hash].consumers <= 0) {
        discardMaterial(hash);
    }

    return 0;
}

VulkanMemoryAllocatorComponent::materialHandle* VulkanMemoryAllocatorComponent::fetchMaterial(Hash_T hash) {
    if(!materials.contains(hash)) hash = DEFAULT_MATERIAL_HASH;

    incrementMaterialConsumers(hash);
    return materials[hash];
}

VulkanMemoryAllocatorComponent::DescriptorPool& VulkanMemoryAllocatorComponent::createDescriptorPool() {
    DescriptorPool& pool = descriptorPools.emplace_back(DESCRIPTORSETSPERPOOL);

    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(DESCRIPTORSETSPERPOOL)
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .maxSets = DESCRIPTORSETSPERPOOL,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    vkCreateDescriptorPool(parent->device->Device, &descriptorPoolCreateInfo, nullptr, &pool.instance);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(DESCRIPTORSETSPERPOOL, parent->pipelines->materialDescriptorSetLayout);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool.instance,
        .descriptorSetCount = DESCRIPTORSETSPERPOOL,
        .pSetLayouts = descriptorSetLayouts.data()
    };

    vkAllocateDescriptorSets(parent->device->Device, &descriptorSetAllocateInfo, &pool.descriptorSets);

    return pool;
}

int VulkanMemoryAllocatorComponent::acquireDescriptorSet() {
    for(int i = 0; i < descriptorPools; i++) {
        if(descriptorPools[i].hasFree()) return descriptorPools[i].getSetIndex() + i * DESCRIPTORSETSPERPOOL;
    }

    logIdentity("No free descriptor sets found, instantiating new pool");

    return createDescriptorPool().getSetIndex() + (descriptorPools.size() - 1) * DESCRIPTORSETSPERPOOL;
}

int VulkanMemoryAllocatorComponent::freeDescriptorSet(int index) {
    int i = index / DESCRIPTORSETSPERPOOL;
    int j = index % DESCRIPTORSETSPERPOOL;

    return descriptorPools[i].releaseSetIndex(j);
}

VkDescriptorSet& VulkanMemoryAllocatorComponent::fetchDescriptorSet(int index) {
    int i = index / DESCRIPTORSETSPERPOOL;
    int j = index % DESCRIPTORSETSPERPOOL;

    if(i > descriptorPools.size()) throw VulkanException("Descriptor set index #" + std::to_string(index) + " resolves to pool #" + std::to_string(i) + " which does not exist");
    if(j > descriptorPools[i].descriptorSets.size()) throw VulkanException("Descriptor set index #" + std::to_string(index) + " resolves to set #" + std::to_string(i) + ":" + std::to_string(j) + " which does not exist");

    return descriptorPools[i].descriptorSets[j];
}

VulkanMemoryAllocatorComponent::DescriptorPool(int _setCount) {
    descriptorSets(_setCount);
    freeDescriptorSetIndices(_setCount);

    for(int i = 0; i < _setCount; i++) {
        freeDescriptorSetIndices[i] = i;
    }
}

bool VulkanMemoryAllocatorComponent::DescriptorPool::hasFree() {
    return freeDescriptorSetIndices.size() > 0;
}

int VulkanMemoryAllocatorComponent::DescriptorPool::getSetIndex() {
    if(freeDescriptorSetIndices.size() == 0) throw VulkanException("A descriptor set was requested from a pool which had no valid sets available");

    int index = freeDescriptorSetIndices[0];
    freeDescriptorSetIndices.erase(freeDescriptorSetIndices.begin());
    return index;
}

int VulkanMemoryAllocatorComponent::DescriptorPool::releaseSetIndex(int index) {
    if(index < 0 || index > descriptorSets.size() || freeDescriptorSetIndices.size() == descriptorSets.size()) return 1;

    bool safely = true;
    if(safely) {
        for(int i = 0; i < freeDescriptorSetIndices.size(); i++) {
            if(index == freeDescriptorSetIndices[i]) logIdentity("Attempted to release a descriptor set index which was already free", 3);
            return 2;
        }
    }

    freeDescriptorSetIndices.push_back(index);
    return 0;
}

bool VulkanMemoryAllocatorComponent::materialHandle::memoryValid(VulkanMemoryAllocatorComponent* allocator) {
    auto check = [&][Hash_T hash] -> bool {
        return textureImageViews.contains(hash) && textureImageViews[hash].pointer->memoryValid(allocator);
    }

    if(!check(ambient)) return false;
    if(!check(diffuse)) return false;
    if(!check(specular)) return false;
    if(!check(specular_highlight)) return false;
    if(!check(bump)) return false;
    if(!check(displacement)) return false;
    if(!check(alpha)) return false;
    if(!check(reflection)) return false;
    return true;
}