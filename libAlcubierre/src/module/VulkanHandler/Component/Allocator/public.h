#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ALLOCATOR_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ALLOCATOR_PUBLIC_H

class VulkanMemoryAllocatorComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

    public:
        VulkanMemoryAllocatorComponent(VulkanHandler* _parent, Registry*& _registry_ptr);
        ~VulkanMemoryAllocatorComponent();

    private:
        //Needs to match staging buffers
        uint32_t VERTEXBUFFERSIZE;
        uint32_t INDEXBUFFERSIZE;
        
        uint32_t GRAPHICSQUEUEINDEX;
        uint32_t TRANSFERQUEUEINDEX;
        uint32_t LOCALMEMORYINDEX;
        uint32_t HOSTMEMORYINDEX;

    public:
        Mesh3D* storeMesh(uint32_t id, std::vector<Vector3> vertices, std::vector<uint32_t> indices);
        int discardMesh(uint32_t id);

        struct bufferSetHandle {
            VkBuffer& vertexBuffer;
            VkBuffer& indexBuffer;
        };

        bufferSetHandle fetchBufferSet(uint32_t id);

        std::string dumpMemoryLayout();

    private:
        class BufferSet {
            public:
                BufferSet(VkDevice& _device, uint32_t _id, uint32_t vertexBufferSize, uint32_t indexBufferSize, uint32_t _queueIndex, uint32_t _memoryTypeIndex);
                ~BufferSet();

                uint32_t id;

            protected:
                struct memoryBlock {
                    memoryBlock(uint32_t _offset, uint32_t _size) : offset(_offset), size(_size) {};
                    
                    uint32_t offset;
                    uint32_t size;
                    uint32_t endIndex() { return offset + size; } //Actually returns the first index AFTER the end of the block
                };

            private:
                class MemoryBuffer {
                    public:
                        MemoryBuffer(VkDevice& _device, VkDeviceMemory& allocation, uint32_t offset, uint32_t _size, uint32_t _queueIndex, bool index = false);
                        ~MemoryBuffer();

                    private:
                        VkDevice& device;
                        VkBuffer instance = VK_NULL_HANDLE;

                        std::vector<memoryBlock> freeBlocks;
                        std::vector<memoryBlock> allocatedBlocks;

                        memoryBlock& largestFreeBlock;
                        memoryBlock& smallestFreeBlock;
                        int recalculateBlockSizes();
                        bool blockCompositionChanged = true;

                    public:
                        memoryBlock* storeMesh(uint32_t id, void* data, uint32_t dataSize);
                        int discardMesh(uint32_t id);

                        VkBuffer& handle();

                        uint32_t largestFreeBlockSize();
                        uint32_t smallestFreeBlockSize();

                        std::string dump();
                };

            private:
                VkDevice& device;

                VkDeviceMemory allocation;

                MemoryBuffer* vertexBuffer = nullptr;
                MemoryBuffer* indexBuffer = nullptr;

                struct meshMemoryLocation {
                    memoryBlock* vertexMemoryBlock;
                    memoryBlock* indexMemoryBlock;
                };
                std::unordered_map<uint32_t, meshMemoryLocation> meshes;

            public:
                int storeMesh(Mesh3D* out, uint32_t id, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices);
                int discardMesh(uint32_t id);

                uint32_t largestFreeVertexBlockSize();
                uint32_t smallestFreeVertexBlockSize();
                uint32_t largestFreeIndexBlockSize();
                uint32_t smallestFreeIndexBlockSize();

                bufferSetHandle handle();

                std::string dump();
        };
        
        std::unordered_map<uint32_t Mesh3D> meshes;

        VkCommandPool stagingCommandPool = VK_NULL_HANDLE;
        VkCommandBuffer stagingCommandBuffer = VK_NULL_HANDLE;
        BufferSet* stagingSet = nullptr;

        std::unordered_map<uint32_t, BufferSet*> bufferSets;
        uint32_t buffersEver = 0;

        BufferSet* instantiateBufferSet();
        BufferSet* pickBufferSet(uint32_t vertexCount, uint32_t indexCount);
};

#endif