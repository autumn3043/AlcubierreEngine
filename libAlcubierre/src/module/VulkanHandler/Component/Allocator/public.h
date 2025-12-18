#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ALLOCATOR_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ALLOCATOR_PUBLIC_H

class VulkanMemoryAllocatorComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

        //Static memory heaps are for meshes, textures, anything persistent and/or medium-large
        //Dynamic memory heaps are for buffers that will be rewritten per frame, so computed geometry or anything with a high update rate
    public:
        class MemoryHeap {
            public:
                struct memoryBlock {
                    uint32_t memoryOffset;
                    uint32_t memorySize;
                    uint32_t memoryEndIndex() { return memoryOffset + memorySize; }

                    memoryBlock(uint32_t _memoryOffset, uint32_t _memorySize) : memoryOffset(_memoryOffset), memorySize(_memorySize) {};
                };

                struct bufferAllocationDetails {
                    MemoryHeap* parent;
                    VkDevice device;
                    memoryBlock allocatedRegion;
                    VkDeviceMemory stagingAllocation;
                    VkBuffer stagingBuffer; //Reference saved so that we can attempt to destruct orphaned memory later
                    VkBuffer executiveBuffer = VK_NULL_HANDLE;
                    uint32_t allocationIndex;

                    bufferAllocationDetails(MemoryHeap* _parent, VkDevice& _device, uint32_t _memoryOffset, uint32_t _memorySize, VkBuffer& _stagingBuffer, VkDeviceMemory& _stagingAllocation, uint32_t _allocationIndex) 
                    : parent(_parent), device(_device), allocatedRegion(_memoryOffset, _memorySize), stagingBuffer(_stagingBuffer), stagingAllocation(_stagingAllocation), allocationIndex(_allocationIndex) {};
                    ~bufferAllocationDetails();
                };

                MemoryHeap(VkDevice& _device, uint32_t deviceLocalMemoryTypeIndex, uint32_t hostVisibleMemoryTypeIndex, uint32_t _heapSize, uint32_t _graphicsComputeQueueIndex);
                ~MemoryHeap();

                //Buffer size specified in bytes
                bufferAllocationDetails& bindBufferMemory(VkBuffer& buffer, uint32_t bufferSize);
                int freeBufferMemory(bufferAllocationDetails& allocation);

                void updateBlockSize(std::vector<memoryBlock>& blocks, uint32_t lower, uint32_t upper);
                uint32_t smallestFreeBlockSize();
                uint32_t largestFreeBlockSize();

            protected:
                std::vector<memoryBlock> freeBlocks;

            private:
                VkDevice& device;
                VkDeviceMemory executiveAllocation;
                VkDeviceMemory stagingAllocation;

                const uint32_t heapSize; //I suppose this places a hard limit on the size of a memory heap at 4.294967GB. Cost of switching to 64 bit ints is probably non-trivial
                const uint32_t graphicsComputeQueueIndex;
                const uint32_t maxFreeBlocks = 32; //A hard capacity limit prevents costly array reallocations, but has obvious drawbacks

                bool blockCompositionChanged = true; //Must be made true at every allocation

                std::vector<bufferAllocationDetails> subAllocations;

                uint32_t largestFreeBlock;
                uint32_t smallestFreeBlock;
        };

    private:
        VulkanDeviceComponent::DeviceQueue* executiveQueue = nullptr;
        VulkanDeviceComponent::DeviceQueue* transferQueue = nullptr;
        VkCommandPool transientCommandPool = VK_NULL_HANDLE;

        uint32_t memoryHeapSize = 8000; //In bytes (all memory sizes are in bytes in this class unless otherwise stated)
        uint32_t deviceLocalMemoryTypeIndex;
        uint32_t hostVisibleMemoryTypeIndex;
        std::vector<MemoryHeap> memoryHeapsStatic;

    public:
        VulkanMemoryAllocatorComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanMemoryAllocatorComponent();

        enum HeapType {
            STATIC = 0,
            DYNAMIC = 1
        };
        MemoryHeap::bufferAllocationDetails& bindBufferMemory(VkBuffer& buffer, uint32_t bufferSize, HeapType type = STATIC);
        //We bind to staging memory, then handle copying to device memory in this component
        int stageBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, void* incomingVerticeData);
        int submitBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, VkFence fence);
        int freeBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation);
};

#endif