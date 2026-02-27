#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ALLOCATOR_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ALLOCATOR_PUBLIC_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <map>

//VkDeviceSize is defined as uint64_t

class VulkanMemoryAllocatorComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

    public:
        VulkanMemoryAllocatorComponent(VulkanHandler* _parent, Registry*& _registry_ptr);
        ~VulkanMemoryAllocatorComponent();

    private:
        //Needs to match staging buffers
        VkDeviceSize VERTEXBUFFERSIZE;
        VkDeviceSize INDEXBUFFERSIZE;
        
        int GRAPHICSQUEUEINDEX;
        int TRANSFERQUEUEINDEX;
        int LOCALMEMORYINDEX;
        int HOSTMEMORYINDEX;

        int WORKERTHREADBUFFERCOUNT;

        VulkanDeviceComponent::QueueHandle* graphicsQueue;
        VulkanDeviceComponent::QueueHandle transferQueue;

    private:
        class MemoryBuffer {
            public:
                MemoryBuffer(VkDevice& _device, VkDeviceSize* size, VkDeviceSize* alignment, uint32_t _queueIndex, VkBufferUsageFlags _usage);
                ~MemoryBuffer();

            public:
                struct memoryBlock {
                    memoryBlock(VkDeviceSize _offset, VkDeviceSize _size) : offset(_offset), size(_size) {};
                    
                    VkDeviceSize offset;
                    VkDeviceSize size;
                    VkDeviceSize endIndex() { return offset + size; } //Actually returns the first index AFTER the end of the block

                    bool operator==(const memoryBlock& other) const { return offset == other.offset; };
                    const memoryBlock operator+(const memoryBlock& other) = delete;

                    memoryBlock(memoryBlock&& other) : offset(other.offset), size(other.size) {};
                    memoryBlock& operator=(memoryBlock&& other) {offset = other.offset; size = other.size; return *this;}
                    memoryBlock(const memoryBlock& other) : offset(other.offset), size(other.size) {}; //We need a copy constructor to return values in acquireMemory
                };

            private:
                VkDevice& device;
                VkBuffer instance = VK_NULL_HANDLE;
                VkDeviceSize alignment;

                std::vector<memoryBlock> freeBlocks;
                std::vector<memoryBlock> allocatedBlocks;

                memoryBlock* largestFreeBlock;
                memoryBlock* smallestFreeBlock;
                int recalculateBlockSizes();
                bool blockCompositionChanged = true;

                int coalesce(int& index);

            public:
                memoryBlock& acquireMemory(VkDeviceSize dataSize);
                int releaseMemory(memoryBlock& block);

                VkBuffer& handle();

                VkDeviceSize largestFreeBlockSize();
                VkDeviceSize smallestFreeBlockSize();

                std::string dump();
        };

    private:
        VkDeviceMemory stagingAllocation = VK_NULL_HANDLE;
        MemoryBuffer* stagingBuffer = nullptr;

    //Meshes
        public:
            int storeMesh(Hash_T hash, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices);
            int discardMesh(Hash_T hash);

            int incrementMeshConsumers(Hash_T hash);
            int decrementMeshConsumers(Hash_T hash);

            bool checkMeshTransferIndexValidity(uint64_t& index);

            struct meshHandle {
                int consumers = 0;

                struct meshHandlePart {
                    VkBuffer* buffer;
                    MemoryBuffer::memoryBlock block;
                    int count;

                    uint64_t memoryValidAfter;
                    bool memoryValidSignal = false;
                    bool memoryValid(VulkanMemoryAllocatorComponent* allocator) { if(!memoryValidSignal) memoryValidSignal = allocator->checkMeshTransferIndexValidity(memoryValidAfter); return memoryValidSignal; };

                    meshHandlePart(VkBuffer* _buffer, int _count, MemoryBuffer::memoryBlock& _block) : buffer(_buffer), count(_count), block(_block) {};
                };
                meshHandlePart vertices;
                meshHandlePart indices;

                bool memoryValid(VulkanMemoryAllocatorComponent* allocator) { return vertices.memoryValid(allocator) && indices.memoryValid(allocator); };

                uint64_t bufferSetId; //Don't use this to query buffer set, renderchain just groups meshHandles.
                //(For now)

                meshHandle(uint64_t _bufferSetId, meshHandlePart _vertices, meshHandlePart _indices) : bufferSetId(_bufferSetId), vertices(_vertices), indices(_indices) {}
            };

            meshHandle* fetchMesh(Hash_T hash);

            std::string dumpMemoryLayout();

        private:
            class BufferSet {
                public:
                    BufferSet(VkDevice& _device, uint64_t _setId, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize, int _queueIndex, int _memoryTypeIndex);
                    ~BufferSet();

                private:
                    VkDevice& device;

                    VkDeviceMemory allocation;

                    MemoryBuffer* vertexBuffer = nullptr;
                    MemoryBuffer* indexBuffer = nullptr;

                    std::unordered_map<Hash_T, MemoryBuffer::memoryBlock> vertexMemoryBlocks;
                    std::unordered_map<Hash_T, MemoryBuffer::memoryBlock> indexMemoryBlocks;

                public:
                    uint64_t setId;

                    meshHandle storeMesh(Hash_T id, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices);
                    int discardMesh(Hash_T id);

                    VkDeviceSize largestFreeVertexBlockSize();
                    VkDeviceSize smallestFreeVertexBlockSize();
                    VkDeviceSize largestFreeIndexBlockSize();
                    VkDeviceSize smallestFreeIndexBlockSize();
                    int storedCount();

                    VkDeviceMemory& allocationHandle();

                    std::string dump();
            };

        private:
            std::unordered_map<Hash_T, meshHandle> meshes;
            std::unordered_map<Hash_T, meshHandle> meshesPendingUpload;
            
            void* hostVisibleMemoryAccess = nullptr;
            char* hostVisibleMemoryAccessBaseIndex = nullptr;

            std::unordered_map<int, BufferSet*> bufferSets;
            uint64_t buffersEver = 0;

            BufferSet* instantiateBufferSet();
            BufferSet* pickBufferSet(VkDeviceSize verticeDataSize, VkDeviceSize indiceDataSize);

    //Textures
        // public:
        //     int storeTexture(Hash_T hash, std::vector<>& texels);
        //     int discardTexture(Hash_T hash);

        //     bool checkTextureTransferIndexValidity(uint64_t& index);

        //     struct textureHandle {
        //     };

        //     textureHandle* fetchTexture(Hash_T hash);

    private:
        struct transferOperation {
            struct region {
                VkBuffer* buffer;
                VkDeviceSize offset;
                VkDeviceSize size;

                region(VkBuffer* _buffer, VkDeviceSize _offset, VkDeviceSize _size) : buffer(_buffer), offset(_offset), size(_size) {}
                region() {}
            };
            region source;
            region destination;

            transferOperation(region _source, region _destination) : source(_source), destination(_destination) {}
            transferOperation() {}
        };

        class transferWorkerThread {
            public:
                transferWorkerThread(VkDevice& _device, VulkanDeviceComponent::QueueHandle* _transferQueue, int bufferCount);
                ~transferWorkerThread();

            private:
                std::thread workerThread;
                bool running = false;

                void thread();

            public:
                uint64_t addTransferOperationToQueue(transferOperation operation);
                uint64_t transferQueuePage();

            private:
                void executeOperation(transferOperation& operation, VkCommandBuffer& buffer, VkFence& fence);

            private:
                uint64_t requestCount = 0;
                VkSemaphore completedCount;
                std::atomic<uint64_t> nextSignalValue{1};

                VkDevice& device;
                VulkanDeviceComponent::QueueHandle* transferQueue = nullptr;

                std::deque<transferOperation> operationQueue;
                std::mutex operationQueue_mutex;
                std::condition_variable operationQueue_conditional;

                VkCommandPool transferCommandPool = VK_NULL_HANDLE;
                std::vector<VkCommandBuffer> commandBuffers;
                std::vector<VkFence> fences;
        };

        transferWorkerThread* worker = nullptr;
};

#endif