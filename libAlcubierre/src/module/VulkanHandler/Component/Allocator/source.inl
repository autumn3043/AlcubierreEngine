VulkanMemoryAllocatorComponent::VulkanMemoryAllocatorComponent(VulkanHandler* _parent, Registry*& _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    VERTEXBUFFERSIZE = CM->Get<int>({"graphics", "vertex_buffer_memory_size_bytes"}, 1000000);
    INDEXBUFFERSIZE = VERTEXBUFFERSIZE * CM->Get<int>({"graphics", "index_buffer_memory_size_ratio"}, 0.25);

    GRAPHICSQUEUEINDEX = &parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT)->familyIndex;
    TRANSFERQUEUEINDEX = &parent->device->getQueue(VK_QUEUE_TRANSFER_BIT)->familyIndex;

    VkPhysicalDeviceMemoryProperties2 physicalDeviceMemory {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
    };
    vkGetPhysicalDeviceMemoryProperties2(parent->device->PhysicalDevice, &physicalDeviceMemory);

    VkMemoryPropertyFlags deviceLocalMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    for(int i = 0; i < physicalDeviceMemory.memoryProperties.memoryTypeCount; i++) {
        if((physicalDeviceMemory.memoryProperties.memoryTypes[i].propertyFlags & deviceLocalMemoryProperties) == deviceLocalMemoryProperties) {
            LOCALMEMORYINDEX = i;
            break;
        }
    }
    VkMemoryPropertyFlags hostVisibleMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for(int i = 0; i < physicalDeviceMemory.memoryProperties.memoryTypeCount; i++) {
        if((physicalDeviceMemory.memoryProperties.memoryTypes[i].propertyFlags & hostVisibleMemoryProperties) == hostVisibleMemoryProperties) {
            HOSTMEMORYINDEX = i;
            break;
        }
    }

    stagingSet = new BufferSet(device, 0, VERTEXBUFFERSIZE, INDEXBUFFERSIZE, TRANSFERQUEUEINDEX, HOSTMEMORYINDEX);
}

VulkanMemoryAllocatorComponent::~VulkanMemoryAllocatorComponent() {
    for(int i = 0; i < bufferSets.size(); i++) {
        if(bufferSets[i]) delete bufferSets[i];
    }
    
    if(stagingBuffer) delete stagingBuffer;
}

Mesh3D VulkanMemoryAllocatorComponent::storeMesh(uint32_t id, std::vector<Vector3> vertices, std::vector<uint32_t> indices) {
    if(!meshes.contains(id)) {
        BufferSet* set = pickBufferSet(vertices.size(), indices.size());
        meshes.emplace(id, { .bufferSetId = id });
        set->storeMesh(&meshes.back(), id, vertices, indices);
    }

    return &meshes[id];
}

int VulkanMemoryAllocatorComponent::discardMesh(uint32_t id) {
    if(meshes.contains(id)) {
        Mesh3D& mesh = meshes[id];
        if(bufferSets.contains(id)) bufferSets[mesh->bufferSetId]->discardMesh(id);
        meshes.
    }

    return 0;
}

bufferSetHandle VulkanMemoryAllocatorComponent::fetchBufferSet(uint32_t id) {
    if(bufferSets.contains(id)) {
        return bufferSets.handle();
    }
}

std::string VulkanMemoryAllocatorComponent::dumpMemoryLayout() {
    std::string hold = "Dumping buffer set layouts ";
    for(int i = 0; i < bufferSets.size(); i++) {
        hold += "(" + std::to_string(i + 1) + "/" + std::to_string(bufferSets.size()) +"):";
        hold += bufferSets[i].dump();
    }

    logIdentity(hold);

    return hold;
}

BufferSet* VulkanMemoryAllocatorComponent::instantiateBufferSet() {
    bufferSets.emplace(buffersEver, new BufferSet(device, buffersEver, VERTEXBUFFERSIZE, INDEXBUFFERSIZE, GRAPHICSQUEUEINDEX, LOCALMEMORYINDEX));
    buffersEver++;

    return bufferSets.end()->second();
}

//Iterates only until a match is found. Best case O(1), worst case O(n).
BufferSet* firstSuitableSet(uint32_t vertexCount, uint32_t indexCount) {
    uint32_t vertexDataSize = vertexCount * sizeof(Vector3);
    uint32_t indexDataSize = indexCount * sizeof(uint32_t);

    for(iterator i = bufferSets.begin(); i != bufferSets.end(); i++) {
        if(i->second()->largestFreeVertexBlockSize() >= vertexDataSize && i->second()->largestFreeIndexBlockSize() >= indexDataSize) {
            return i->second();
        }
    }

    return nullptr;
}

//Always iterates over every buffer. O(n).
BufferSet* smallestSet(uint32_t vertexCount, uint32_t indexCount) {
    uint32_t vertexDataSize = vertexCount * sizeof(Vector3);
    uint32_t indexDataSize = indexCount * sizeof(uint32_t);

    BufferSet* r = nullptr;
    uint32_t smallestFoundFreeVertexBlockSize = UINT32_MAX;

    for(iterator i = bufferSets.begin(); i != bufferSets.end(); i++) {
        if(i->second()->largestFreeVertexBlockSize() < vertexDataSize || i->second()->largestFreeIndexBlockSize() < indexDataSize) continue;

        uint32_t& size = i->second()->smallestFreeVertexBlockSize();
        if(size < smallestFoundFreeVertexBlockSize) {
            r = i->second();
            smallestFoundFreeVertexBlockSize = size;
        }
    }

    return r;
}

BufferSet* VulkanMemoryAllocatorComponent::pickBufferSet(uint32_t vertexCount, uint32_t indexCount) {
    BufferSet* r;
    r = firstSuitableSet(vertexCount, indexCount);
    //r = smallestSet(vertexCount, indexCount);

    if(!r) return instantiateBufferSet();
    return r;
}

VulkanMemoryAllocatorComponent::BufferSet::BufferSet(VkDevice& _device, uint32_t _id, uint32_t vertexBufferSize, uint32_t indexBufferSize, uint32_t _queueIndex, uint32_t _memoryTypeIndex) : device(_device), id(_id) {
    VkMemoryAllocateInfo allocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = vertexBufferSize + indexBufferSize,
        .memoryTypeIndex = _memoryTypeIndex
    };

    vkAllocateMemory(device, &allocationInfo, nullptr, &allocation);

    vertexBuffer = new MemoryBuffer(device, allocation, vertexBufferSize, 0, _queueIndex);
    indexBuffer = new MemoryBuffer(device, allocation, indexBufferSize, vertexBufferSize, _queueIndex, true);
}

VulkanMemoryAllocatorComponent::BufferSet::~BufferSet() {
    if(vertexBuffer) delete vertexBuffer;
    if(indexBuffer) delete indexBuffer;
    vkFreeMemory(device, allocation, nullptr);
}

int VulkanMemoryAllocatorComponent::BufferSet::storeMesh(Mesh3D* out, uint32_t id, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices) {
    meshes.emplace(id, {vertexBuffer.storeMesh(vertices.data(), sizeof(Vector3) * vertices.size()), indexBuffer.storeMesh(indices.data, sizeof(uint32_t) * indices.size())});
    out->vertexOffset = meshes[id].vertexMemoryBlock->offset;
    out->vertexCount = vertices.size();
    out->indexOffset = meshes[id].indexMemoryBlock->offset;
    out->indexCount = indices.size():
    return 0;
}

uint32_t VulkanMemoryAllocatorComponent::BufferSet::largestFreeVertexBlockSize() {
    return vertexBuffer->largestFreeBlock();
}

uint32_t VulkanMemoryAllocatorComponent::BufferSet::smallestFreeVertexBlockSize() {
    return vertexBuffer->smallestFreeBlock();
}

uint32_t VulkanMemoryAllocatorComponent::BufferSet::largestFreeIndexBlockSize() {
    return indexBuffer->largestFreeBlock();
}

uint32_t VulkanMemoryAllocatorComponent::BufferSet::smallestFreeIndexBlockSize() {
    return indexBuffer->smallestFreeBlock();
}

bufferSetHandle VulkanMemoryAllocatorComponent::BufferSet::handle() {
    return bufferSetHandle{vertexBuffer->handle(), indexBuffer->handle()}
}

std::string VulkanMemoryAllocatorComponent::BufferSet::dump() {
    std::string hold;
    hold += "\nVertex buffer:"
    hold += vertexBuffer.dump();
    hold += "\nIndex buffer:"
    hold += indexBuffer.dump();
    return hold;
}

VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::MemoryBuffer(VkDevice& _device, VkDeviceMemory& allocation, uint32_t offset, uint32_t _size, uint32_t _queueIndex, bool index) : device(_device) {
    VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = _size,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &_queueIndex
    };

    if(!index) createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    vkCreateBuffer(device, &createInfo, nullptr, &instance);
    vkBindBufferMemory(device, instance, allocation, offset);
}

VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::~MemoryBuffer() {
    if(instance) vkDestroyBuffer(device, instance, nullptr);
}

memoryBlock* VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::storeMesh(uint32_t id, void* data, uint32_t dataSize) {
    //store it
    memoryBlock* temp;

    meshes.emplace(id, temp);
    return temp;
}

int VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::recalculateBlockSizes() {
    largestFreeBlock = smallestFreeBlock = freeBlocks[0];

    for(int i = 1; i < freeBlocks.size(); i++) {
        uint32_t& size = freeBlocks[i].size;
        if(size > largestFreeBlock.size) largestFreeBlock = freeBlocks[i];
        else if(size < smallestFreeBlock.size) smallestFreeBlock = freeBlocks[i];
    }

    blockCompositionChanged = false;
    return 0;
}

VkBuffer& VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::handle() {
    return instance;
}

uint32_t VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::largestFreeBlockSize() {
    if(blockCompositionChanged) recalculateBlockSizes();
    return largestFreeBlock.size;
}

uint32_t VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::smallestFreeBlockSize() {
    if(blockCompositionChanged) recalculateBlockSizes();
    return smallestFreeBlock.size;
}

std::string VulkanMemoryAllocatorComponent::BufferSet::MemoryBuffer::dump() {
    std::map<uint32_t, std::string> memoryMap;
    for(int i = 0; i < allocatedBlocks.size(); i++) {
        memoryBlock& block = allocatedBlocks[i];
        memoryMap.emplace(block.offset, std::string("\n") +
            "Allocated #" + std::to_string(i) + ": " +
            "begin " + std::to_string(block.offset) + "; "
            "end " + std::to_string(block.endIndex()) + "; "
            "size " + std::to_string(block.size) + ";"
        );
    }

    for(int i = 0; i < freeBlocks.size(); i++) {
        memoryBlock& block = freeBlocks[i];
        memoryMap.emplace(block.offset, std::string("\n") +
            "Free #" + std::to_string(i) + ": " +
            "begin " + std::to_string(block.offset) + "; "
            "end " + std::to_string(block.endIndex()) + "; "
            "size " + std::to_string(block.size) + ";"
        );
    }

    std::string hold;
    for(int i = 0; i < memoryMap.size(); i++) {
        hold += memoryMap[i];
    }
    return hold;
}