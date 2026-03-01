int VulkanMemoryAllocatorComponent::initTextureHandler() {
    return 0;
}

int VulkanMemoryAllocatorComponent::destroyTextureHandler() {
    return 0;
}

int VulkanMemoryAllocatorComponent::storeTexture(Hash_T hash) {
    return 0;
}

int VulkanMemoryAllocatorComponent::discardTexture(Hash_T hash) {
    return 0;
}

int VulkanMemoryAllocatorComponent::incrementTextureConsumers(Hash_T hash) {
    return 0;
}
int VulkanMemoryAllocatorComponent::decrementTextureConsumers(Hash_T hash) {
    return 0;
}

bool VulkanMemoryAllocatorComponent::checkTextureTransferIndexValidity(uint64_t& index) {
    return 0;
}

VulkanMemoryAllocatorComponent::textureHandle* VulkanMemoryAllocatorComponent::fetchTexture(Hash_T hash) {
    return 0;
}