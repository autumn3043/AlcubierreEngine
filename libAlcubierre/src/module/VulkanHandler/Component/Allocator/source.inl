VulkanMemoryAllocatorComponent::VulkanMemoryAllocatorComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    //Get a reference to the queues that the device deems appropriate
        executiveQueue = &parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT);
        transferQueue = &parent->device->getQueue(VK_QUEUE_TRANSFER_BIT);

    //Find the indices of our preferred memory types. Usually device local (faster, cannot be mapped) for executive buffers and host visible (can be mapped) for staging buffers 
        VkPhysicalDeviceMemoryProperties2 physicalDeviceMemory {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
        };
        vkGetPhysicalDeviceMemoryProperties2(parent->device->PhysicalDevice, &physicalDeviceMemory);

        VkMemoryPropertyFlags deviceLocalMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        for(int i = 0; i < physicalDeviceMemory.memoryProperties.memoryTypeCount; i++) {
            if((physicalDeviceMemory.memoryProperties.memoryTypes[i].propertyFlags & deviceLocalMemoryProperties) == deviceLocalMemoryProperties) {
                deviceLocalMemoryTypeIndex = i;
                break;
            }
        }

        VkMemoryPropertyFlags hostVisibleMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        for(int i = 0; i < physicalDeviceMemory.memoryProperties.memoryTypeCount; i++) {
            if((physicalDeviceMemory.memoryProperties.memoryTypes[i].propertyFlags & hostVisibleMemoryProperties) == hostVisibleMemoryProperties) {
                hostVisibleMemoryTypeIndex = i;
                break;
            }
        }

    //Create a command pool suited to multiple short-lived command buffers which just execute a copy operation
        VkCommandPoolCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = transferQueue->familyIndex
        };

        VkResult commandPoolCreateResult = vkCreateCommandPool(parent->device->Device, &createInfo, nullptr, &transientCommandPool);

        if(commandPoolCreateResult != VK_SUCCESS) {
            throw VulkanException("Failed to create a command pool for copy operations, encountered VkResult code " + std::to_string(commandPoolCreateResult));
        }
    
    //TEMP: Just create a single static heap and use that for everything for now
        memoryHeapsStatic.emplace_back(parent->device->Device, deviceLocalMemoryTypeIndex, hostVisibleMemoryTypeIndex, memoryHeapSize, executiveQueue->familyIndex);
}

VulkanMemoryAllocatorComponent::~VulkanMemoryAllocatorComponent() {
    if(transientCommandPool) vkDestroyCommandPool(parent->device->Device, transientCommandPool, nullptr);
}

VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails& VulkanMemoryAllocatorComponent::bindBufferMemory(VkBuffer& buffer, uint32_t bufferSize, HeapType type) {
    uint32_t heapIndex = 0;

    for(uint32_t i = 1; i < memoryHeapsStatic.size(); i++) {
        //Find the existing heap with the smallest free block which is large enough for this buffer. If there are none, create a new one
        if(memoryHeapsStatic[i].largestFreeBlockSize() > bufferSize) heapIndex = i;
    }

    return memoryHeapsStatic[heapIndex].bindBufferMemory(buffer, bufferSize);
}

int VulkanMemoryAllocatorComponent::stageBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, void* incomingVerticeData) {
    void* hostMemoryBuffer;
    vkMapMemory(parent->device->Device, bufferAllocation.stagingAllocation, bufferAllocation.allocatedRegion.memoryOffset, bufferAllocation.allocatedRegion.memorySize, NULL_BIT, &hostMemoryBuffer);
    memcpy(hostMemoryBuffer, incomingVerticeData, bufferAllocation.allocatedRegion.memorySize);
    vkUnmapMemory(parent->device->Device, bufferAllocation.stagingAllocation);
    return 0;
}

int VulkanMemoryAllocatorComponent::submitBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, VkFence fence = VK_NULL_HANDLE) {
    //TODO: Cull non-unique vertices in some intelligent way, or decide if the renderchain is going to do that. Either way do something

    //Create a temporary command buffer
        VkCommandBufferAllocateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = transientCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(parent->device->Device, &createInfo, &commandBuffer);

    //Record to that command buffer a copy buffer operation
        VkCommandBufferBeginInfo beginInfo { 
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = NULL_BIT,
            .pInheritanceInfo = nullptr
        };

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy region { .srcOffset = 0, .dstOffset = 0, .size = bufferAllocation.allocatedRegion.memorySize };
        vkCmdCopyBuffer(commandBuffer, bufferAllocation.stagingBuffer, bufferAllocation.executiveBuffer, 1, &region);

        vkEndCommandBuffer(commandBuffer);

    //Submit command buffer to transfer queue
        VkSubmitInfo SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer
        };

        if(fence) vkQueueSubmit(transferQueue->queue, 1, &SubmitInfo, fence);
        else vkQueueSubmit(transferQueue->queue, 1, &SubmitInfo, nullptr);

    //Now the app has a reference to a device-local command buffer whose contents have been copied from its corresponding host-visible command buffer. It should now pass this to vkCmdBindVertexBuffers in order to render these vertices
    return 0;
}

int VulkanMemoryAllocatorComponent::freeBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation) {
    bufferAllocation.parent->freeBufferMemory(bufferAllocation);
    return 0;
}

VulkanMemoryAllocatorComponent::MemoryHeap::MemoryHeap(VkDevice& _device, uint32_t deviceLocalMemoryTypeIndex, uint32_t hostVisibleMemoryTypeIndex, uint32_t _heapSize, uint32_t _graphicsComputeQueueIndex) 
    : device(_device), heapSize(_heapSize), graphicsComputeQueueIndex(_graphicsComputeQueueIndex) {

    //Allocate device local memory space for the actual executive buffer
    VkMemoryAllocateInfo executiveAllocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = heapSize,
        .memoryTypeIndex = deviceLocalMemoryTypeIndex
    };
    VkResult executiveResult = vkAllocateMemory(device, &executiveAllocationInfo, nullptr, &executiveAllocation);

    //And host visible memory for the staging buffer the application may submit data to
    VkMemoryAllocateInfo stagingAllocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = heapSize,
        .memoryTypeIndex = hostVisibleMemoryTypeIndex
    };
    VkResult stagingResult = vkAllocateMemory(device, &stagingAllocationInfo, nullptr, &stagingAllocation);

    if((executiveResult == VK_SUCCESS) && (stagingResult == VK_SUCCESS)) {
        DM().Log("Allocated a static memory heap sized " + std::to_string(heapSize) + " bytes");
    } else {
        std::string result;
        if(executiveResult != VK_SUCCESS) result += std::to_string(executiveResult);
        if((executiveResult != VK_SUCCESS) && (stagingResult != VK_SUCCESS)) result += " and ";
        if(stagingResult != VK_SUCCESS) result += std::to_string(stagingResult);
        DM().Log("Failed to allocate a static memory heap sized " + std::to_string(heapSize) + " bytes due to VkResult: " + result, 3);
    }

    subAllocations.reserve(maxFreeBlocks);
    freeBlocks.reserve(maxFreeBlocks);
    freeBlocks.emplace_back(memoryBlock(0, heapSize));
    smallestFreeBlock = heapSize;
    largestFreeBlock = heapSize;
}

VulkanMemoryAllocatorComponent::MemoryHeap::~MemoryHeap() {
    if(subAllocations.size() > 0) {
        DM().Log("Destructing orphaned sub-allocations, which should not be necessary when buffers are correctly freed at the end of their lifespan");
        subAllocations.clear();
    }

    vkFreeMemory(device, stagingAllocation, nullptr);
    vkFreeMemory(device, executiveAllocation, nullptr);
}

void VulkanMemoryAllocatorComponent::MemoryHeap::updateBlockSize(std::vector<memoryBlock>& blocks, uint32_t lower, uint32_t upper) {
    lower = blocks[0].memorySize;
    upper = blocks[0].memorySize;

    for(int i = 1; i < blocks.size(); i++) {
        uint32_t size = blocks[i].memorySize;
        if(size < lower) lower = size;
        if(size > upper) upper = size;
    }
}

uint32_t VulkanMemoryAllocatorComponent::MemoryHeap::smallestFreeBlockSize() {
    if(blockCompositionChanged) {
        updateBlockSize(freeBlocks, smallestFreeBlock, largestFreeBlock);
        blockCompositionChanged = false;
    }

    return smallestFreeBlock;
}

uint32_t VulkanMemoryAllocatorComponent::MemoryHeap::largestFreeBlockSize() {
    if(blockCompositionChanged) {
        updateBlockSize(freeBlocks, smallestFreeBlock, largestFreeBlock);
        blockCompositionChanged = false;
    }

    return largestFreeBlock;
}

VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails& VulkanMemoryAllocatorComponent::MemoryHeap::bindBufferMemory(VkBuffer& buffer, uint32_t bufferSize) {
    //TODO: Replace throws with indicative error numbers, and free corresponding memory when an issue occurs in assigning it

    //Find and reserve a suitably sized memory region
        if(bufferSize > largestFreeBlockSize()) throw VulkanException("Submitted buffer does not fit inside this heap. Should never happen because the parent allocator should check this");

        //Use block 0 as base case, start indexing from block 1.
        uint32_t smallestFoundIndex = 0;
        uint32_t smallestFoundSize = freeBlocks[0].memorySize;
        for(uint32_t i = 1; i < freeBlocks.size(); i++) {
            uint32_t size = freeBlocks[i].memorySize;
            if(size < smallestFoundSize && size >= bufferSize) {
                smallestFoundIndex = i;
                smallestFoundSize = size;
            }
        }

        if(freeBlocks.size() >= maxFreeBlocks) throw VulkanException("No remaining space in heap block stack (maxFreeBlocks = " + std::to_string(maxFreeBlocks) + ")");

        uint32_t smallestFoundOffset = freeBlocks[smallestFoundIndex].memoryOffset;
        //Doing this step last would mean that memory is only actually allocated on a successful binding, otherwise an error will result in permanently unused memory, but I'm slightly concerned that might lead to a race condition
        freeBlocks.erase(freeBlocks.begin() + smallestFoundIndex);
        freeBlocks.emplace_back(smallestFoundOffset + bufferSize, smallestFoundSize - bufferSize);
        blockCompositionChanged = true;

    //Bind that region in staging memory to the supplied buffer
        VkResult stagingBindResult = vkBindBufferMemory(device, buffer, stagingAllocation, smallestFoundOffset);

        if(stagingBindResult != VK_SUCCESS) {
            throw VulkanException("Failed to bind buffer to memory index " + std::to_string(smallestFoundOffset) + " VkResult code: " + std::to_string(stagingBindResult));
        }

    //Note the allocated region and its buffer
        uint32_t allocationIndex = subAllocations.size();
        bufferAllocationDetails& newAllocation = subAllocations.emplace_back(this, device, smallestFoundOffset, bufferSize, buffer, stagingAllocation, allocationIndex);

    //Create a matching executive buffer
        VkBufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphicsComputeQueueIndex
        };

        VkResult executiveCreateResult = vkCreateBuffer(device, &createInfo, nullptr, &newAllocation.executiveBuffer);

        if(executiveCreateResult != VK_SUCCESS) {
            throw VulkanException("Failed to create matching executive buffer at memory index " + std::to_string(smallestFoundOffset) + " VkResult code: " + std::to_string(executiveCreateResult));
        }

    
    //Bind the executive buffer to the matching region in executive memory
        VkResult executiveBindResult = vkBindBufferMemory(device, newAllocation.executiveBuffer, executiveAllocation, smallestFoundOffset);

        if(executiveBindResult != VK_SUCCESS) {
            throw VulkanException("Failed to bind buffer to memory index " + std::to_string(smallestFoundOffset) + " VkResult code: " + std::to_string(executiveBindResult));
        }

    return newAllocation;
}

int VulkanMemoryAllocatorComponent::MemoryHeap::freeBufferMemory(bufferAllocationDetails& allocation) {
    subAllocations.erase(subAllocations.begin() + allocation.allocationIndex);

    return 0;
}

VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails::~bufferAllocationDetails() {
    if(device != VK_NULL_HANDLE) {
        // if(stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, stagingBuffer, nullptr);
        if(executiveBuffer) vkDestroyBuffer(device, executiveBuffer, nullptr);
    } else {
        // DM().Log("Tried to destruct allocated buffer, but VkDevice handle was null, which would have caused a segfault. This memory will leak", 2);
    }

    //Check for adjacent free blocks in memory and coalesce
        uint32_t previousAdjacentEndIndex = allocatedRegion.memoryOffset - 1;
        memoryBlock* previousBlockPtr = nullptr;
        uint32_t previousBlockIndex;
        uint32_t nextAdjacentStartIndex = allocatedRegion.memoryEndIndex() + 1;
        memoryBlock* nextBlockPtr = nullptr;
        uint32_t nextBlockIndex;

        for(uint32_t i = 0; i < parent->freeBlocks.size(); i++) {
            memoryBlock& block = parent->freeBlocks[i];

            if(block.memoryEndIndex() == previousAdjacentEndIndex) {
                previousBlockPtr = &parent->freeBlocks[i];
                previousBlockIndex = i;
            }
            if(block.memoryOffset == nextAdjacentStartIndex) {
                nextBlockPtr = &parent->freeBlocks[i];
                nextBlockIndex = i;
            }

            if(previousBlockPtr && nextBlockPtr) break;
        }

        uint32_t newBlockStartIndex = allocatedRegion.memoryOffset;
        if(previousBlockPtr) {
            newBlockStartIndex = previousBlockPtr->memoryOffset;
            parent->freeBlocks.erase(parent->freeBlocks.begin() + previousBlockIndex);
        }

        uint32_t newBlockEndIndex = allocatedRegion.memoryEndIndex();
        if(nextBlockPtr) {
            newBlockEndIndex = nextBlockPtr->memoryEndIndex();
            parent->freeBlocks.erase(parent->freeBlocks.begin() + nextBlockIndex);
        }

        parent->freeBlocks.emplace_back(newBlockStartIndex, (newBlockEndIndex - newBlockStartIndex));
}