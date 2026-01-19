#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H

class VulkanRenderchainComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

        struct Vertex {
            float position[2];
            float colour[3];
        };
        class VertexBuffer {
            public:
                VertexBuffer(VulkanMemoryAllocatorComponent* _allocator, VkDevice& _device, uint32_t _vertexCount, uint32_t _indexCount, uint32_t transferQueueIndex, int _index);
                ~VertexBuffer();

                const uint32_t vertexCount;
                const uint32_t vertex_t_size;
                const uint32_t indexCount;
                const uint32_t index_t_size;
                const uint32_t bufferSize = vertexCount * vertex_t_size + indexCount * index_t_size;
                const uint32_t bufferIndexBreakpointOffset = vertexCount * vertex_t_size;

                int fillBufferMemory(void* data, uint32_t dataSize);

                VkDevice& device;
                VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails* allocation;

                int index;

            private:
                VkBuffer bufferInstance;

                VulkanMemoryAllocatorComponent* allocator;
        };

        std::vector<VertexBuffer*> vertexBuffersInMemory;
        std::vector<int> vertexBuffersInFrame;

        int CreateGraphicsPipelines();
            class graphicsPipeline {
                public:
                    graphicsPipeline();
                    ~graphicsPipeline();
                    
                    virtual int init(VkDevice& _device, VkFormat& format) = 0;
                    virtual int recordPass(VkCommandBuffer& commandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image, std::vector<VertexBuffer*>& buffersInMemory, std::vector<int>& buffersInFrame) = 0;

                protected:
                    VkDevice device = VK_NULL_HANDLE;

                    VkPipeline pipeline = VK_NULL_HANDLE;
                    VkPipelineLayout layout = VK_NULL_HANDLE;

                    VkShaderModule shaderModule = VK_NULL_HANDLE;
            };

            class opaqueGeometryPipe : public graphicsPipeline {
                public:
                    int init(VkDevice& _device, VkFormat& format) override;
                    int recordPass(VkCommandBuffer& commandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image, std::vector<VertexBuffer*>& buffersInMemory, std::vector<int>& buffersInFrame) override;
            };
            opaqueGeometryPipe pipeline_geometry;


        int CreateCommandPools();
            class CommandPool;

            struct CommandBuffer {
                VkDevice& device;

                CommandBuffer(VkDevice& _device, VkCommandPool& commandPool, int _timeout);
                ~CommandBuffer();

                const int timeout;
                
                VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
                VkSemaphore semaphore = VK_NULL_HANDLE;
                VkFence fence = VK_NULL_HANDLE;
            };

            class CommandPool {
                public:
                    CommandPool(VkDevice& _device, int queueIndex, int bufferCount, int commandTimeoutSeconds);
                    ~CommandPool();

                    std::vector<CommandBuffer> buffers;

                private:
                    VkDevice& device;

                    int bufferCount;
                    int commandTimeout;

                    VkCommandPool commandPool = VK_NULL_HANDLE;

            };
            CommandPool* graphicalCommandPool = nullptr;
            CommandPool* transferCommandPool = nullptr;

        int RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image, std::vector<graphicsPipeline*> pipelines);
        
    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanRenderchainComponent();

        int drawFrame();

        int maxFramesInFlight = 0;
        int currentFrame = 0;

        int createObjectBuffer(int*& modelBufferIndex, IGraphicsBackend::modelData& data);
        int addObjectToFrame(int& modelBufferIndex, IGraphicsBackend::placementData& data);
        int discardObjectBuffer(int& modelBufferIndex);
        int clearFrame();
};

#endif