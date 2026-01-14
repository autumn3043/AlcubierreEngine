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
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(parent->device->Device, buffer, &requirements);
    uint32_t allocatingBufferSize = ((bufferSize + requirements.alignment - 1) / requirements.alignment) * requirements.alignment;

    if(allocatingBufferSize > memoryHeapSize) throw VulkanException("Requested buffer bind size was " + std::to_string(allocatingBufferSize) + " bytes which is more than the largest available memory heap size (" + std::to_string(memoryHeapSize) + " bytes)");

    uint32_t heapIndex = 0;
    uint32_t foundHeapBlockSize = memoryHeapsStatic[0].largestFreeBlockSize();

    for(uint32_t i = 1; i < memoryHeapsStatic.size(); i++) {
        //Find the existing heap with the smallest free block which is large enough for this buffer. If there are none, create a new one
        uint32_t thisHeapSize = memoryHeapsStatic[i].largestFreeBlockSize();
        if(thisHeapSize > allocatingBufferSize && thisHeapSize < foundHeapBlockSize) {
            heapIndex = i;
            foundHeapBlockSize = thisHeapSize;
        } else if(i + 1 == memoryHeapsStatic.size()) {
            memoryHeapsStatic.emplace_back(parent->device->Device, deviceLocalMemoryTypeIndex, hostVisibleMemoryTypeIndex, memoryHeapSize, executiveQueue->familyIndex);
            heapIndex = i + 1;
            break;
        }
    }

    return memoryHeapsStatic[heapIndex].bindBufferMemory(buffer, bufferSize, allocatingBufferSize);
}

int VulkanMemoryAllocatorComponent::stageBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, void* data, uint32_t dataSize) {
    void* hostMemoryBuffer;
    vkMapMemory(parent->device->Device, bufferAllocation.stagingAllocation, bufferAllocation.allocatedRegion.memoryOffset, bufferAllocation.allocatedRegion.memorySize, NULL_BIT, &hostMemoryBuffer);
    std::byte* hostBufferIndex = static_cast<std::byte*>(hostMemoryBuffer);
    memcpy(hostBufferIndex, data, dataSize);
    vkUnmapMemory(parent->device->Device, bufferAllocation.stagingAllocation);
    return 0;
}

int VulkanMemoryAllocatorComponent::submitBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, VkFence fence = VK_NULL_HANDLE) {
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
        
        VkBufferCopy region { .srcOffset = 0, .dstOffset = 0, .size = bufferAllocation.usedRegion.memorySize };
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

void VulkanMemoryAllocatorComponent::dump() {
    std::string hold = "Dumping memory heap layouts ";
    for(int i = 0; i < memoryHeapsStatic.size(); i++) {
        hold += "(" + std::to_string(i + 1) + "/" + std::to_string(memoryHeapsStatic.size()) +"):";
        hold += memoryHeapsStatic[i].printMemoryLayout();
    }
    logIdentity(hold);
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
        logIdentity("Allocated a static memory heap sized " + std::to_string(heapSize) + " bytes");
    } else {
        std::string result;
        if(executiveResult != VK_SUCCESS) result += std::to_string(executiveResult);
        if((executiveResult != VK_SUCCESS) && (stagingResult != VK_SUCCESS)) result += " and ";
        if(stagingResult != VK_SUCCESS) result += std::to_string(stagingResult);
        logIdentity("Failed to allocate a static memory heap sized " + std::to_string(heapSize) + " bytes due to VkResult: " + result, 3);
    }

    subAllocations.reserve(maxFreeBlocks);
    freeBlocks.reserve(maxFreeBlocks);
    freeBlocks.emplace_back(memoryBlock(0, heapSize));
    smallestFreeBlock = heapSize;
    largestFreeBlock = heapSize;
}

VulkanMemoryAllocatorComponent::MemoryHeap::~MemoryHeap() {
    if(subAllocations.size() > 0) {
        logIdentity("Destructing orphaned sub-allocations, which should not be necessary when buffers are correctly freed at the end of their lifespan");
        subAllocations.clear();
    }

    vkFreeMemory(device, stagingAllocation, nullptr);
    vkFreeMemory(device, executiveAllocation, nullptr);
    logIdentity("Successfully freed memory heap staging & executive allocations");
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

std::string VulkanMemoryAllocatorComponent::MemoryHeap::printAllocation(uint32_t index) {
    return std::string("\n") +
        "Alloc #" + std::to_string(index) + ": " +
        "begin " + std::to_string(subAllocations[index].allocatedRegion.memoryOffset) + "; "
        "end " + std::to_string(subAllocations[index].allocatedRegion.memoryEndIndex()) + "; "
        "size " + std::to_string(subAllocations[index].allocatedRegion.memorySize) + ";";
}

std::string VulkanMemoryAllocatorComponent::MemoryHeap::printFree(uint32_t index) {
    return std::string("\n") +
        "Free #" + std::to_string(index) + ": " +
        "begin " + std::to_string(freeBlocks[index].memoryOffset) + "; "
        "end " + std::to_string(freeBlocks[index].memoryEndIndex()) + "; "
        "size " + std::to_string(freeBlocks[index].memorySize) + ";";
}

#include <map>
std::string VulkanMemoryAllocatorComponent::MemoryHeap::printMemoryLayout() {
    std::map<uint32_t, std::string> memoryMap; //begin index, print
    for(int i = 0; i < subAllocations.size(); i++) {
        memoryMap.emplace(subAllocations[i].allocatedRegion.memoryOffset, printAllocation(i));
    }
    for(int i = 0; i < freeBlocks.size(); i++) {
        memoryMap.emplace(freeBlocks[i].memoryOffset, printFree(i));
    }

    std::string hold;
    for(int i = 0; i < memoryMap.size(); i++) {
        hold += memoryMap[i];
    }
    return hold;
}

VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails& VulkanMemoryAllocatorComponent::MemoryHeap::bindBufferMemory(VkBuffer& buffer, uint32_t bufferSize, uint32_t realBufferSize) {
    //TODO: Replace throws with indicative error numbers, and free corresponding memory when an issue occurs in assigning it

    //Find and reserve a suitably sized memory region
        if(realBufferSize > largestFreeBlockSize()) throw VulkanException("Submitted buffer does not fit inside this heap. Should never happen because the parent allocator should check this");

        //Use block 0 as base case, start indexing from block 1.
        uint32_t smallestFoundIndex = 0;
        uint32_t smallestFoundSize = freeBlocks[0].memorySize;
        for(uint32_t i = 1; i < freeBlocks.size(); i++) {
            uint32_t size = freeBlocks[i].memorySize;
            if(size < smallestFoundSize && size >= realBufferSize) {
                smallestFoundIndex = i;
                smallestFoundSize = size;
            }
        }

        if(freeBlocks.size() >= maxFreeBlocks) throw VulkanException("No remaining space in heap block stack (maxFreeBlocks = " + std::to_string(maxFreeBlocks) + ")");

        uint32_t smallestFoundOffset = freeBlocks[smallestFoundIndex].memoryOffset;
        //Doing this step last would mean that memory is only actually allocated on a successful binding, otherwise an error will result in permanently unused memory, but I'm slightly concerned that might lead to a race condition
        freeBlocks.erase(freeBlocks.begin() + smallestFoundIndex);
        freeBlocks.emplace_back(smallestFoundOffset + realBufferSize, smallestFoundSize - realBufferSize);
        blockCompositionChanged = true;

    //Bind that region in staging memory to the supplied buffer
        VkResult stagingBindResult = vkBindBufferMemory(device, buffer, stagingAllocation, smallestFoundOffset);

        if(stagingBindResult != VK_SUCCESS) {
            throw VulkanException("Failed to bind buffer to memory index " + std::to_string(smallestFoundOffset) + " VkResult code: " + std::to_string(stagingBindResult));
        }

    //Note the allocated region and its buffer
        uint32_t allocationIndex = subAllocations.size();
        bufferAllocationDetails& newAllocation = subAllocations.emplace_back(this, device, stagingAllocation, buffer, smallestFoundOffset, bufferSize, realBufferSize, allocationIndex);

    //Create a matching executive buffer
        VkBufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = realBufferSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
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

    logIdentity("Successfully created and bound empty memory buffer sized " + std::to_string(realBufferSize) + " bytes at offset " + std::to_string(smallestFoundOffset));
    return newAllocation;
}

int VulkanMemoryAllocatorComponent::MemoryHeap::freeBufferMemory(bufferAllocationDetails& allocation) {
    allocation.cleanup();
    subAllocations.erase(subAllocations.begin() + allocation.allocationIndex);

    return 0;
}

void VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails::cleanup() {
    if(device != VK_NULL_HANDLE) {
        // if(stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, stagingBuffer, nullptr);
        if(executiveBuffer) vkDestroyBuffer(device, executiveBuffer, nullptr);
    } else {
        // logIdentity("Tried to destruct allocated buffer, but VkDevice handle was null, which would have caused a segfault. This memory will leak", 2);
    }

    //Check for adjacent free blocks in memory and coalesce
        memoryBlock* nextBlockPtr = nullptr;
        uint32_t nextBlockIndex;
        memoryBlock* previousBlockPtr = nullptr;
        uint32_t previousBlockIndex;

        for(uint32_t i = 0; i < parent->freeBlocks.size(); i++) {
            memoryBlock& block = parent->freeBlocks[i];

            if(block.memoryOffset == allocatedRegion.memoryEndIndex()) {
                nextBlockPtr = &parent->freeBlocks[i];
                nextBlockIndex = i;
            }
            if(block.memoryEndIndex() == allocatedRegion.memoryOffset) {
                previousBlockPtr = &parent->freeBlocks[i];
                previousBlockIndex = i;
            }

            if(nextBlockPtr && previousBlockPtr) break;
        }

        uint32_t newBlockEndIndex = allocatedRegion.memoryEndIndex();
        if(nextBlockPtr) {
            newBlockEndIndex = nextBlockPtr->memoryEndIndex();
            parent->freeBlocks.erase(parent->freeBlocks.begin() + nextBlockIndex);
        }

        uint32_t newBlockStartIndex = allocatedRegion.memoryOffset;
        if(previousBlockPtr) {
            newBlockStartIndex = previousBlockPtr->memoryOffset;
            parent->freeBlocks.erase(parent->freeBlocks.begin() + previousBlockIndex);
        }

        parent->freeBlocks.emplace_back(newBlockStartIndex, (newBlockEndIndex - newBlockStartIndex));
}