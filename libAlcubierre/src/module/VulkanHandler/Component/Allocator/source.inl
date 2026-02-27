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
    worker = new transferWorkerThread(parent->device->Device, &transferQueue, WORKERTHREADBUFFERCOUNT);

    //Staging buffer
    VkDeviceSize stagingBufferSize = VERTEXBUFFERSIZE;
    stagingBuffer = new MemoryBuffer(parent->device->Device, &stagingBufferSize, nullptr, TRANSFERQUEUEINDEX, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    VkMemoryAllocateInfo allocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = stagingBufferSize,
        .memoryTypeIndex = static_cast<uint32_t>(HOSTMEMORYINDEX)
    };

    vkAllocateMemory(parent->device->Device, &allocationInfo, nullptr, &stagingAllocation);
    vkBindBufferMemory(parent->device->Device, stagingBuffer->handle(), stagingAllocation, 0);
    vkMapMemory(parent->device->Device, stagingAllocation, 0, VK_WHOLE_SIZE, NULL_BIT, &hostVisibleMemoryAccess);
    hostVisibleMemoryAccessBaseIndex = static_cast<char*>(hostVisibleMemoryAccess);

    instantiateBufferSet();
}

VulkanMemoryAllocatorComponent::~VulkanMemoryAllocatorComponent() {
    if(worker) delete worker;

    for(int i = 0; i < bufferSets.size(); i++) {
        if(bufferSets.at(i)) delete bufferSets.at(i);
    }

    vkUnmapMemory(parent->device->Device, stagingAllocation);
    vkFreeMemory(parent->device->Device, stagingAllocation, nullptr);
    if(stagingBuffer) delete stagingBuffer;
}

int VulkanMemoryAllocatorComponent::storeMesh(Hash_T hash, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices) {
    if(meshes.contains(hash)) return 1;

    VkDeviceSize verticeDataSize = vertices.size() * sizeof(Vector3);
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
        transferOperation::region(stagingMemory.vertices.buffer, stagingMemory.vertices.block.offset, stagingMemory.vertices.block.size), 
        transferOperation::region(deviceMemory.vertices.buffer, deviceMemory.vertices.block.offset, deviceMemory.vertices.block.size)
    ));
    deviceMemory.indices.memoryValidAfter = worker->addTransferOperationToQueue(transferOperation(
        transferOperation::region(stagingMemory.indices.buffer, stagingMemory.indices.block.offset, stagingMemory.indices.block.size), 
        transferOperation::region(deviceMemory.indices.buffer, deviceMemory.indices.block.offset, deviceMemory.indices.block.size)
    ));

    dumpMemoryLayout();

    return 0;
}

int VulkanMemoryAllocatorComponent::discardMesh(Hash_T hash) {
    if(!meshes.contains(hash)) return 1;
    
    bufferSets.at(meshes.at(hash).bufferSetId)->discardMesh(hash);
    meshes.erase(hash);

    dumpMemoryLayout();
    
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

std::string VulkanMemoryAllocatorComponent::dumpMemoryLayout() {
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

VulkanMemoryAllocatorComponent::meshHandle VulkanMemoryAllocatorComponent::BufferSet::storeMesh(Hash_T id, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices) {
    VkDeviceSize verticesSize = sizeof(Vector3) * vertices.size();
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

VulkanMemoryAllocatorComponent::MemoryBuffer::MemoryBuffer(VkDevice& _device, VkDeviceSize* size, VkDeviceSize* alignment, uint32_t _queueIndex, VkBufferUsageFlags _usage) : device(_device) {
    VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = *size,
        .usage = _usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &_queueIndex
    };

    vkCreateBuffer(device, &createInfo, nullptr, &instance);
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, instance, &requirements);
    if(size) *size = requirements.size; 
    if(alignment) *alignment = requirements.alignment;

    freeBlocks.emplace_back(0, *size);
}

VulkanMemoryAllocatorComponent::MemoryBuffer::~MemoryBuffer() {
    if(instance) vkDestroyBuffer(device, instance, nullptr);
}

VulkanMemoryAllocatorComponent::MemoryBuffer::memoryBlock& VulkanMemoryAllocatorComponent::MemoryBuffer::acquireMemory(VkDeviceSize dataSize) {
    VkDeviceSize smallestFoundIndex = 0;
    VkDeviceSize& smallestFoundSize = freeBlocks[0].size;
    for(int i = 1; i < freeBlocks.size(); i++) {
        VkDeviceSize& size = freeBlocks[i].size;
        if(size < smallestFoundSize && size >= dataSize) {
            smallestFoundIndex = i;
            smallestFoundSize = size;
        }
    }

    VkDeviceSize remainder = freeBlocks[smallestFoundIndex].size - dataSize;
    assert(remainder >= 0);

    allocatedBlocks.emplace_back(freeBlocks[smallestFoundIndex].offset, dataSize);

    if(remainder > 0) {
        freeBlocks[smallestFoundIndex].offset += dataSize;
        freeBlocks[smallestFoundIndex].size -= dataSize;
    } else {
        freeBlocks.erase(freeBlocks.begin() + smallestFoundIndex);
    }

    blockCompositionChanged = true;
    return allocatedBlocks.back();
}

int VulkanMemoryAllocatorComponent::MemoryBuffer::releaseMemory(memoryBlock& block) {
    int eraseIndex;
    int insertIndex;

    for(int i = 0; i < allocatedBlocks.size(); i++) {
        if(allocatedBlocks[i] != block) continue;
        eraseIndex = i;
        break;
    }

    if(freeBlocks.size() == 0) insertIndex = 0;
    bool found = false;
    for(int j = 0; j < freeBlocks.size(); j++) {
        if(j == 0) {
            if(freeBlocks[j].offset >= block.endIndex()) {
                insertIndex = j;
                found = true;
            }
        } else if(j == freeBlocks.size() - 1) {
            if(freeBlocks[j - 1].endIndex() <= block.offset) {
                insertIndex = j;
                found = true;
            }
        } else {
            if(freeBlocks[j - 1].endIndex() <= block.offset && freeBlocks[j].offset >= block.endIndex()) {
                insertIndex = j;
                found = true;
            }
        }

        if(found) break;
    }

    freeBlocks.insert(freeBlocks.begin() + insertIndex, block);
    allocatedBlocks.erase(allocatedBlocks.begin() + eraseIndex);
    coalesce(insertIndex);

    return 0;
}

int VulkanMemoryAllocatorComponent::MemoryBuffer::recalculateBlockSizes() {
    largestFreeBlock = smallestFreeBlock = &freeBlocks[0];

    for(int i = 1; i < freeBlocks.size(); i++) {
        VkDeviceSize& size = freeBlocks[i].size;
        if(size > largestFreeBlock->size) largestFreeBlock = &freeBlocks[i];
        else if(size < smallestFreeBlock->size) smallestFreeBlock = &freeBlocks[i];
    }

    blockCompositionChanged = false;
    return 0;
}

int VulkanMemoryAllocatorComponent::MemoryBuffer::coalesce(int& index) {
    VkDeviceSize offset = freeBlocks[index].offset;
    VkDeviceSize end = freeBlocks[index].endIndex();

    bool previousAdjacentIsFree = false;
    bool nextAdjacentIsFree = false;

    if(index > 0) previousAdjacentIsFree = (freeBlocks[index - 1].endIndex() == offset);
    if(index < freeBlocks.size() - 1) nextAdjacentIsFree = (freeBlocks[index + 1].offset == end);
    if(!previousAdjacentIsFree && !nextAdjacentIsFree) return 1;

    int newIndex = index;
    if(previousAdjacentIsFree) { offset = freeBlocks[index - 1].offset; newIndex -= 1; }
    if(nextAdjacentIsFree) end = freeBlocks[index + 1].endIndex();

    memoryBlock newBlock(offset, end - offset);
    freeBlocks.insert(freeBlocks.begin() + newIndex, std::move(newBlock));
    freeBlocks.erase(freeBlocks.begin() + newIndex + 1, freeBlocks.begin() + newIndex + 2 + previousAdjacentIsFree + nextAdjacentIsFree);
    //erase is [i, j), i.e. it will erase i but not j, so add 1 to the entry index because we insert the new block before everything else, then add 2 to the end index so that j is 1 past the end.
    
    blockCompositionChanged = true;
    return 0;    
}

VkBuffer& VulkanMemoryAllocatorComponent::MemoryBuffer::handle() {
    return instance;
}

VkDeviceSize VulkanMemoryAllocatorComponent::MemoryBuffer::largestFreeBlockSize() {
    if(blockCompositionChanged) recalculateBlockSizes();
    return largestFreeBlock->size;
}

VkDeviceSize VulkanMemoryAllocatorComponent::MemoryBuffer::smallestFreeBlockSize() {
    if(blockCompositionChanged) recalculateBlockSizes();
    return smallestFreeBlock->size;
}

#include <algorithm>

std::string VulkanMemoryAllocatorComponent::MemoryBuffer::dump() {
    //OPT: This is a convenient way to sort because std::sort uses T::operator<() by default, but I don't love defining a struct inside a method. 
    struct memoryBlockDebugInfo {
        VkDeviceSize* offset;
        std::string info;

        bool operator<(memoryBlockDebugInfo& other) { return *offset < *other.offset; }
    };

    std::vector<memoryBlockDebugInfo> allMemoryBlocks(allocatedBlocks.size() + freeBlocks.size());

    for(int i = 0; i < allocatedBlocks.size(); i++) {
        allMemoryBlocks[i].offset = &allocatedBlocks[i].offset;
        allMemoryBlocks[i].info = (std::string("\n   ") +
            "Allocated #" + std::to_string(i) + ": " +
            "begin " + std::to_string(allocatedBlocks[i].offset) + "; "
            "end " + std::to_string(allocatedBlocks[i].endIndex()) + "; "
            "size " + std::to_string(allocatedBlocks[i].size) + "B;"
        );
    }
    for(int i = 0; i < freeBlocks.size(); i++) {
        allMemoryBlocks[i + allocatedBlocks.size()].offset = &freeBlocks[i].offset;
        allMemoryBlocks[i + allocatedBlocks.size()].info = (std::string("\n   ") +
            "Free #" + std::to_string(i) + ": " +
            "begin " + std::to_string(freeBlocks[i].offset) + "; "
            "end " + std::to_string(freeBlocks[i].endIndex()) + "; "
            "size " + std::to_string(freeBlocks[i].size) + "B;"
        );
    }

    std::sort(allMemoryBlocks.begin(), allMemoryBlocks.end());

    std::string hold;
    for(int i = 0; i < allMemoryBlocks.size(); i++) {
        hold += allMemoryBlocks[i].info;
    }

    return hold;
}

VulkanMemoryAllocatorComponent::transferWorkerThread::transferWorkerThread(VkDevice& _device, VulkanDeviceComponent::QueueHandle* _transferQueue, int bufferCount) : device(_device), transferQueue(_transferQueue) {
    VkCommandPoolCreateInfo poolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = transferQueue->deviceQueueFamilyIndex
    };
    vkCreateCommandPool(device, &poolCreateInfo, nullptr, &transferCommandPool);

    VkCommandBufferAllocateInfo bufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = transferCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(bufferCount)
    };
    commandBuffers.resize(bufferCount);
    vkAllocateCommandBuffers(device, &bufferCreateInfo, commandBuffers.data());

    VkFenceCreateInfo fenceCreateInfo { 
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    fences.resize(bufferCount);
    for(int i = 0; i < bufferCount; i++) {
        vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]);
    }
    
    VkSemaphoreTypeCreateInfo semaphoreTypeInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0
    };
    VkSemaphoreCreateInfo semaphoreCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphoreTypeInfo,
        .flags = NULL_BIT
    };
    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &completedCount);

    running = true;
    workerThread = std::thread(&transferWorkerThread::thread, this);
}

VulkanMemoryAllocatorComponent::transferWorkerThread::~transferWorkerThread() {
    running = false;
    operationQueue_conditional.notify_all();
    if(workerThread.joinable()) workerThread.join();
    vkWaitForFences(device, fences.size(), fences.data(), VK_TRUE, UINT64_MAX);
    for(VkFence& fence : fences) vkDestroyFence(device, fence, nullptr);
    vkDestroySemaphore(device, completedCount, nullptr);
    vkDestroyCommandPool(device, transferCommandPool, nullptr);
}

uint64_t VulkanMemoryAllocatorComponent::transferWorkerThread::addTransferOperationToQueue(transferOperation operation) {
    {
        std::lock_guard<std::mutex> lock(operationQueue_mutex);
        operationQueue.push_back(operation);
        operationQueue_conditional.notify_one();
    }

    requestCount++;
    logIdentity("Added transfer operation request #" + std::to_string(requestCount - 1));
    return requestCount - 1;
}

uint64_t VulkanMemoryAllocatorComponent::transferWorkerThread::transferQueuePage() {
    uint64_t r;
    vkGetSemaphoreCounterValue(device, completedCount, &r);
    return r;
}

// #include <iostream>

void VulkanMemoryAllocatorComponent::transferWorkerThread::thread() {
    while(running) {

        //Wait for operation in queue
        transferOperation currentOperation;

        {
            std::unique_lock<std::mutex> lock(operationQueue_mutex);
            operationQueue_conditional.wait(lock, [&]{ return !operationQueue.empty() || !running; });

            // std::cout << "Transfer worker thread woke from sleep" << std::endl;

            if(!running) break;

            currentOperation = operationQueue.front();
            operationQueue.pop_front();
        }
        
        if(!running) break;

        //Get or await idle buffer
        vkWaitForFences(device, fences.size(), fences.data(), VK_FALSE, UINT64_MAX);
        for(int i = 0; i < fences.size(); i++) {
            if(vkGetFenceStatus(device, fences[i]) != VK_SUCCESS) continue;
            executeOperation(currentOperation, commandBuffers[i], fences[i]);
            break;
        }
    }
}

void VulkanMemoryAllocatorComponent::transferWorkerThread::executeOperation(transferOperation& operation, VkCommandBuffer& buffer, VkFence& fence) {
    vkResetFences(device, 1, &fence);
    vkResetCommandBuffer(buffer, NULL_BIT);

    VkCommandBufferBeginInfo beginInfo { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(buffer, &beginInfo);
    
    VkBufferCopy region { .srcOffset = operation.source.offset, .dstOffset = operation.destination.offset, .size = operation.source.size };
    // std::cout << operation.source.size << std::endl;
    vkCmdCopyBuffer(buffer, *operation.source.buffer, *operation.destination.buffer, 1, &region);

    vkEndCommandBuffer(buffer);

    uint64_t semaphoreValue = nextSignalValue.fetch_add(1);
    VkTimelineSemaphoreSubmitInfo semaphoreSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &semaphoreValue
    };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &semaphoreSubmitInfo,
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &completedCount
    };

    vkQueueSubmit(transferQueue->queue, 1, &submitInfo, fence);
}