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
                    uint32_t memoryEndIndex() { return memoryOffset + memorySize; } //Actually returns the first index AFTER the end of the block

                    memoryBlock(uint32_t _memoryOffset, uint32_t _memorySize) : memoryOffset(_memoryOffset), memorySize(_memorySize) {};
                };

                struct bufferAllocationDetails {
                    MemoryHeap* parent;

                    VkDevice device;
                    VkDeviceMemory stagingAllocation;
                    VkBuffer stagingBuffer; //Reference saved so that we can attempt to destruct orphaned memory later
                    VkBuffer executiveBuffer = VK_NULL_HANDLE;

                    uint32_t indiceOffset;

                    memoryBlock usedRegion;
                    memoryBlock allocatedRegion;
                    uint32_t allocationIndex;

                    bufferAllocationDetails(MemoryHeap* _parent, VkDevice& _device, VkDeviceMemory& _stagingAllocation, VkBuffer& _stagingBuffer, uint32_t _memoryOffset, uint32_t _usedMemorySize, uint32_t _allocatedMemorySize, uint32_t _allocationIndex) 
                    : parent(_parent), device(_device), stagingAllocation(_stagingAllocation), stagingBuffer(_stagingBuffer), usedRegion(_memoryOffset, _usedMemorySize), allocatedRegion(_memoryOffset, _allocatedMemorySize), allocationIndex(_allocationIndex) {};

                    void cleanup();
                };

                MemoryHeap(VkDevice& _device, uint32_t deviceLocalMemoryTypeIndex, uint32_t hostVisibleMemoryTypeIndex, uint32_t _heapSize, uint32_t _graphicsComputeQueueIndex);
                ~MemoryHeap();

                //Buffer size specified in bytes
                bufferAllocationDetails& bindBufferMemory(VkBuffer& buffer, uint32_t usedBufferSize, uint32_t allocatedBufferSize);
                int freeBufferMemory(bufferAllocationDetails& allocation);

                void updateBlockSize(std::vector<memoryBlock>& blocks, uint32_t lower, uint32_t upper);
                uint32_t smallestFreeBlockSize();
                uint32_t largestFreeBlockSize();

                std::string printMemoryLayout();

            protected:
                std::vector<memoryBlock> freeBlocks;
                std::vector<bufferAllocationDetails> subAllocations;

            private:
                VkDevice& device;
                VkDeviceMemory executiveAllocation;
                VkDeviceMemory stagingAllocation;

                const uint32_t heapSize; //I suppose this places a hard limit on the size of a memory heap at 4.294967GB. Cost of switching to 64 bit ints is probably non-trivial
                const uint32_t graphicsComputeQueueIndex;
                const uint32_t maxFreeBlocks = 64; //A hard capacity limit prevents costly array reallocations, but has obvious drawbacks

                bool blockCompositionChanged = true; //Must be made true at every allocation

                uint32_t largestFreeBlock;
                uint32_t smallestFreeBlock;

                std::string printAllocation(uint32_t index);
                std::string printFree(uint32_t index);
        };

    private:
        VulkanDeviceComponent::DeviceQueue* executiveQueue = nullptr;
        VulkanDeviceComponent::DeviceQueue* transferQueue = nullptr;
        VkCommandPool transientCommandPool = VK_NULL_HANDLE;

        uint32_t memoryHeapSize = 1000000; //In bytes (all memory sizes are in bytes in this class unless otherwise stated)
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
        int stageBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, void* data, uint32_t dataSize);
        int submitBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation, VkFence fence);
        int freeBufferMemory(MemoryHeap::bufferAllocationDetails& bufferAllocation);

        void dump();
};

std::vector<BufferSet> bufferSets;

int loadMeshToBuffer(Mesh3D* mesh, std::vector<Vector>& vertices, std::vector<uint32_t>& indices);
//Find or create best buffer pair
//Write buffer id to mesh.bufferId
//Dump vertices and indices to pair
//Write vertex and index offset to mesh.xOffset
int discardMesh(Mesh3D* mesh);
BufferSet* fetchBufferSet(int bufferId);

#endif