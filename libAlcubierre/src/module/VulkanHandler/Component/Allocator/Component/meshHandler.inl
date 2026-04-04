int VulkanMemoryAllocatorComponent::initMeshHandler() {
    instantiateBufferSet();

    return 0;
}

int VulkanMemoryAllocatorComponent::destroyMeshHandler() {
    for(int i = 0; i < bufferSets.size(); i++) {
        if(bufferSets.at(i)) delete bufferSets.at(i);
    }

    return 0;
}

int VulkanMemoryAllocatorComponent::storeMesh(Hash_T hash, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    if(meshes.contains(hash)) return 1;

    VkDeviceSize verticeDataSize = vertices.size() * sizeof(Vertex);
    VkDeviceSize indiceDataSize = indices.size() * sizeof(uint32_t);
    BufferSet* set = pickBufferSet(verticeDataSize, indiceDataSize);
    meshHandle& deviceMemory = meshes.emplace(hash, set->storeMesh(hash, vertices, indices)).first->second;

    //Check all meshes pending upload in buffers. If they have been completed, we can free that part of the staging buffer. 
    //OPT: Find some way to call back to this or include it in the worker thread or something. 
    //OPT: If members are stored in order of their memory valid index then we can do something like release all it.begin() -> x where x is the last element where memoryValid()
    //FIX: Erases member from list currently being iterated.
    if(meshesPendingUpload.size() > 0) {
        for(std::unordered_map<Hash_T, meshHandle>::iterator it = meshesPendingUpload.begin(); it != meshesPendingUpload.end();) {
            if(it->second.memoryValid(this)) {
                stagingBuffer->releaseMemory(it->second.vertices.block);
                stagingBuffer->releaseMemory(it->second.indices.block);
                it = meshesPendingUpload.erase(it);
            } else {
                it++;
            }
        }
    }
    //We need to acquire a region of staging buffer memory to guarantee it for the duration of the copy operation.
    meshHandle& stagingMemory = meshesPendingUpload.emplace(
        hash, 
        meshHandle(
            0,
            meshHandle::meshHandlePart(&stagingBuffer->handle(), vertices.size(), stagingBuffer->acquireMemory(verticeDataSize)),
            meshHandle::meshHandlePart(&stagingBuffer->handle(), indices.size(), stagingBuffer->acquireMemory(indiceDataSize))
        )
    ).first->second;

    //Copy into the staging memory.
    memcpy(hostVisibleMemoryAccessBaseIndex + stagingMemory.vertices.block.offset, vertices.data(), verticeDataSize);
    memcpy(hostVisibleMemoryAccessBaseIndex + stagingMemory.indices.block.offset, indices.data(), indiceDataSize);

    //From here, add copy operations to pool queue without blocking and mark when done.
    deviceMemory.vertices.memoryValidAfter = worker->addTransferOperationToQueue(transferOperation(
        TRANSFER_OPERATION_TYPE_BUFFER,
        transferOperation::region(stagingMemory.vertices.buffer, stagingMemory.vertices.block.offset, stagingMemory.vertices.block.size), 
        transferOperation::region(deviceMemory.vertices.buffer, deviceMemory.vertices.block.offset, deviceMemory.vertices.block.size)
    ));
    deviceMemory.indices.memoryValidAfter = worker->addTransferOperationToQueue(transferOperation(
        TRANSFER_OPERATION_TYPE_BUFFER,
        transferOperation::region(stagingMemory.indices.buffer, stagingMemory.indices.block.offset, stagingMemory.indices.block.size), 
        transferOperation::region(deviceMemory.indices.buffer, deviceMemory.indices.block.offset, deviceMemory.indices.block.size)
    ));

    dumpMeshMemoryLayout();

    return 0;
}

int VulkanMemoryAllocatorComponent::discardMesh(Hash_T hash) {
    if(!meshes.contains(hash)) return 1;
    
    bufferSets.at(meshes.at(hash).bufferSetId)->discardMesh(hash);
    meshes.erase(hash);

    dumpMeshMemoryLayout();
    
    return 0;
}

int VulkanMemoryAllocatorComponent::incrementMeshConsumers(Hash_T hash) {
    if(!meshes.contains(hash)) return 1;

    meshes.at(hash).consumers++;

    return 0;
}

int VulkanMemoryAllocatorComponent::decrementMeshConsumers(Hash_T hash) {
    if(!meshes.contains(hash)) return 1;

    meshHandle& mesh = meshes.at(hash);
    mesh.consumers--;
    if(mesh.consumers == 0) {
        discardMesh(hash);
        return 2;
    }

    return 0;
}

bool VulkanMemoryAllocatorComponent::checkMeshTransferIndexValidity(uint64_t& index) {
    return worker->transferQueuePage() >= index;
}

VulkanMemoryAllocatorComponent::meshHandle* VulkanMemoryAllocatorComponent::fetchMesh(Hash_T hash) {
    // logIdentity(std::to_string(hash));
    assert(meshes.contains(hash));
    return &meshes.at(hash);
}

std::string VulkanMemoryAllocatorComponent::dumpMeshMemoryLayout() {
    std::string hold = "Dumping buffer set layouts ";
    for(int i = 0; i < bufferSets.size(); i++) {
        hold += "(" + std::to_string(i + 1) + "/" + std::to_string(bufferSets.size()) +"):";
        hold += bufferSets.at(i)->dump();
    }

    logIdentity(hold);

    return hold;
}

VulkanMemoryAllocatorComponent::BufferSet* VulkanMemoryAllocatorComponent::instantiateBufferSet() {
    BufferSet* set = bufferSets.emplace(buffersEver, new BufferSet(parent->device->Device, buffersEver, VERTEXBUFFERSIZE, INDEXBUFFERSIZE, GRAPHICSQUEUEINDEX, LOCALMEMORYINDEX)).first->second;
    buffersEver++;

    return set;
}

VulkanMemoryAllocatorComponent::BufferSet* VulkanMemoryAllocatorComponent::pickBufferSet(VkDeviceSize verticeDataSize, VkDeviceSize indiceDataSize) {
    if(verticeDataSize > VERTEXBUFFERSIZE) throw VulkanException("Mesh sized " + std::to_string(verticeDataSize) + "B" + " will never fit in memory buffers of max size " + std::to_string(VERTEXBUFFERSIZE) + "B");
    if(indiceDataSize > INDEXBUFFERSIZE) throw VulkanException("Mesh sized " + std::to_string(indiceDataSize) + "B" + " will never fit in memory buffers of max size " + std::to_string(INDEXBUFFERSIZE) + "B");

    BufferSet* r = nullptr;
    VkDeviceSize smallestFoundFreeVertexBlockSize = bufferSets.begin()->second->smallestFreeVertexBlockSize() + 1;

    for(std::unordered_map<int, BufferSet*>::iterator it = bufferSets.begin(); it != bufferSets.end(); it++) {
        if(it->second->largestFreeVertexBlockSize() < verticeDataSize || it->second->largestFreeIndexBlockSize() < indiceDataSize) continue;

        VkDeviceSize size = it->second->smallestFreeVertexBlockSize();
        if(size < smallestFoundFreeVertexBlockSize) {
            r = it->second;
            smallestFoundFreeVertexBlockSize = size;
        }
    }

    if(!r) return instantiateBufferSet();
    else return r;
}

VulkanMemoryAllocatorComponent::BufferSet::BufferSet(VkDevice& _device, uint64_t _setId, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize, int _queueIndex, int _memoryTypeIndex) : device(_device), setId(_setId) {
    logIdentity("Creating buffer set with VBuffer size " + std::to_string(vertexBufferSize) + "B" + " and IBuffer size " + std::to_string(indexBufferSize) + "B");

    VkDeviceSize v_size = vertexBufferSize;
    VkDeviceSize v_alignment;
    vertexBuffer = new MemoryBuffer(device, &v_size, &v_alignment, _queueIndex, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkDeviceSize i_size = indexBufferSize;
    VkDeviceSize i_alignment;
    indexBuffer = new MemoryBuffer(device, &i_size, &i_alignment, _queueIndex, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    VkDeviceSize i_offset = ((v_size + i_alignment - 1) / i_alignment) * i_alignment; //Cancelling multiplication to truncate decimal

    VkMemoryAllocateInfo allocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = i_offset + i_size,
        .memoryTypeIndex = static_cast<uint32_t>(_memoryTypeIndex)
    };

    vkAllocateMemory(device, &allocationInfo, nullptr, &allocation);

    vkBindBufferMemory(device, vertexBuffer->handle(), allocation, 0);
    vkBindBufferMemory(device, indexBuffer->handle(), allocation, i_offset);
}

VulkanMemoryAllocatorComponent::BufferSet::~BufferSet() {
    if(vertexBuffer) delete vertexBuffer;
    if(indexBuffer) delete indexBuffer;
    vkFreeMemory(device, allocation, nullptr);
}

VulkanMemoryAllocatorComponent::meshHandle VulkanMemoryAllocatorComponent::BufferSet::storeMesh(Hash_T id, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    VkDeviceSize verticesSize = sizeof(Vertex) * vertices.size();
    assert(verticesSize < largestFreeVertexBlockSize());
    VkDeviceSize indicesSize = sizeof(uint32_t) * indices.size();
    assert(indicesSize < largestFreeIndexBlockSize());

    vertexMemoryBlocks.emplace(id, vertexBuffer->acquireMemory(verticesSize));
    indexMemoryBlocks.emplace(id, indexBuffer->acquireMemory(indicesSize));

    return meshHandle(
        setId,
        meshHandle::meshHandlePart(&vertexBuffer->handle(), vertices.size(), vertexMemoryBlocks.at(id)),
        meshHandle::meshHandlePart(&indexBuffer->handle(), indices.size(), indexMemoryBlocks.at(id))
    );
}

int VulkanMemoryAllocatorComponent::BufferSet::discardMesh(Hash_T id) {
    assert(vertexMemoryBlocks.contains(id) && indexMemoryBlocks.contains(id));
    vertexBuffer->releaseMemory(vertexMemoryBlocks.at(id));
    vertexMemoryBlocks.erase(id);
    indexBuffer->releaseMemory(indexMemoryBlocks.at(id));
    indexMemoryBlocks.erase(id);

    return 0;
}

VkDeviceSize VulkanMemoryAllocatorComponent::BufferSet::largestFreeVertexBlockSize() {
    return vertexBuffer->largestFreeBlockSize();
}

VkDeviceSize VulkanMemoryAllocatorComponent::BufferSet::smallestFreeVertexBlockSize() {
    return vertexBuffer->smallestFreeBlockSize();
}

VkDeviceSize VulkanMemoryAllocatorComponent::BufferSet::largestFreeIndexBlockSize() {
    return indexBuffer->largestFreeBlockSize();
}

VkDeviceSize VulkanMemoryAllocatorComponent::BufferSet::smallestFreeIndexBlockSize() {
    return indexBuffer->smallestFreeBlockSize();
}

int VulkanMemoryAllocatorComponent::BufferSet::storedCount() {
    return vertexMemoryBlocks.size();
}

VkDeviceMemory& VulkanMemoryAllocatorComponent::BufferSet::allocationHandle() {
    return allocation;
}

std::string VulkanMemoryAllocatorComponent::BufferSet::dump() {
    std::string hold;
    hold += "\nVertex buffer:";
    hold += vertexBuffer->dump();
    hold += "\nIndex buffer:";
    hold += indexBuffer->dump();
    return hold;
}