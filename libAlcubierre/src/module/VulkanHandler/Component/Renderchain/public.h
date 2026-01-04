#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H

#include <cassert>

class VulkanRenderchainComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;
        
        int CreateGraphicsPipeline();
            struct ShaderModule {
                ShaderModule(VkDevice& _device);
                ~ShaderModule();

                VkDevice& device;
                
                VkShaderModule module = VK_NULL_HANDLE;
            };

            struct ShaderModuleStage {
                ShaderModuleStage(int _index, std::string _name, std::string _type, ShaderModule& shader);
                ~ShaderModuleStage() = default;

                int index;
                std::string name;
                VkShaderStageFlagBits type;
                VkPipelineShaderStageCreateInfo createInfo;
            };

            class GraphicsPipeline {
                public:
                    GraphicsPipeline(VkDevice& _device, VkFormat& format, std::vector<ShaderModuleStage>& shaderStages);
                    ~GraphicsPipeline();

                    const VkDevice& device;

                    VkPipeline pipeline = VK_NULL_HANDLE;
                    VkPipelineLayout layout = VK_NULL_HANDLE;
            };
        GraphicsPipeline* pipeline = nullptr;

        int CreateTransferCommandPool();
        int CreateGraphicalCommandPool();
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

        struct Vertex {
            float position[2];
            float colour[3];
        };
        class VertexBuffer {
            public:
                VertexBuffer(VulkanMemoryAllocatorComponent* _allocator, VkDevice& _device, uint32_t _vertexCount, uint32_t _indexCount, uint32_t transferQueueIndex, uint32_t _modelHash);
                ~VertexBuffer();

                const uint32_t vertexCount;
                const uint32_t vertex_t_size;
                const uint32_t indexCount;
                const uint32_t index_t_size;
                const uint32_t bufferSize = vertexCount * vertex_t_size + indexCount * index_t_size;
                const uint32_t bufferIndexBreakpointOffset = vertexCount * vertex_t_size;

                int fillBufferMemory(std::vector<Vertex>& vertexBuffer, std::vector<uint32_t>& indexBuffer);

                VkDevice& device;
                VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails* allocation;

                const uint32_t modelHash;

            private:
                VkBuffer bufferInstance;

                VulkanMemoryAllocatorComponent* allocator;
        };
        std::vector<VertexBuffer*> vertexBuffersInMemory;
        std::vector<uint32_t> vertexBuffersInFrame;

        int RecreateSwapchain();
        int numberOfFrames = 0;
        int RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image);
        
    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanRenderchainComponent();

        int createObjectBuffer(uint32_t& modelHash, std::vector<Vector>& vertices, std::vector<uint32_t>& indices);
        int clearFrame();
        int addObjectToFrame(uint32_t& modelHash, Vector& position);
        int drawFrame();

        int maxFramesInFlight = 0;
        int currentFrame = 0;
};

#endif