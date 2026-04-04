VulkanMemoryAllocatorComponent::VulkanMemoryAllocatorComponent(VulkanHandler* _parent, Registry*& _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    
    VERTEXBUFFERSIZE = static_cast<VkDeviceSize>(CM->Get<int>({"graphics", "vertex_buffer_memory_size_bytes"}, 1000000));
    INDEXBUFFERSIZE = VERTEXBUFFERSIZE * (static_cast<float>(sizeof(uint32_t)) / static_cast<float>(sizeof(Vector3)) * CM->Get<float>({"graphics", "index_buffer_memory_size_ratio"}, 1.00f));

    graphicsQueue = &parent->graphicsRenderingQueue;
    transferQueue = parent->device->fetchQueueHandle(VulkanDeviceComponent::queueType::TRANSFER);

    GRAPHICSQUEUEINDEX = graphicsQueue->deviceQueueFamilyIndex;
    TRANSFERQUEUEINDEX = transferQueue.deviceQueueFamilyIndex;

    LOCALMEMORYINDEX = parent->device->fetchDeviceProperties().memory.deviceLocalIndex;
    HOSTMEMORYINDEX = parent->device->fetchDeviceProperties().memory.hostVisibleIndex;

    WORKERTHREADBUFFERCOUNT = CM->Get<int>({"graphics", "transfer_buffer_count"}, 8);
    
    DESCRIPTORSETSPERPOOL = CM->Get<int>({"graphics", "descriptor_sets_per_pool_count"}, 8);

    initTransferHandler();
    initMeshHandler();
    initTextureHandler();
}

VulkanMemoryAllocatorComponent::~VulkanMemoryAllocatorComponent() {
    destroyTextureHandler();
    destroyMeshHandler();
    destroyTransferHandler();
}

#include "module/VulkanHandler/Component/Allocator/Component/transferHandler.inl"
#include "module/VulkanHandler/Component/Allocator/Component/meshHandler.inl"
#include "module/VulkanHandler/Component/Allocator/Component/textureHandler.inl"