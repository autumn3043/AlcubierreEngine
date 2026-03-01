int VulkanMemoryAllocatorComponent::initTransferHandler() {
    worker = new transferWorkerThread(parent->device->Device, &transferQueue, WORKERTHREADBUFFERCOUNT);

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

    return 0;
}

int VulkanMemoryAllocatorComponent::destroyTransferHandler() {
    if(worker) delete worker;

    vkUnmapMemory(parent->device->Device, stagingAllocation);
    vkFreeMemory(parent->device->Device, stagingAllocation, nullptr);
    if(stagingBuffer) delete stagingBuffer;
    
    return 0;
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