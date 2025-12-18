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

                // private:
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

        int CreateVertexBuffers();
            struct Vertex {
                float position[2];
                float colour[3];
            };
            class VertexBuffer {
                public:
                    VertexBuffer(VulkanMemoryAllocatorComponent* _allocator, VkDevice& _device, uint32_t _bufferSize, uint32_t transferQueueIndex);
                    ~VertexBuffer();

                    int fillBufferMemory(std::vector<Vertex>& external_membuffer);

                    VkDevice& device;
                    VulkanMemoryAllocatorComponent::MemoryHeap::bufferAllocationDetails* allocation;

                private:
                    VkBuffer bufferInstance;
                    uint32_t bufferSize;

                    VulkanMemoryAllocatorComponent* allocator;
            };
        std::vector<VertexBuffer*> vertexBuffers;
        std::vector<Vertex> vertices_temp;

        int RecreateSwapchain();
        int RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image);
        
    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanRenderchainComponent();

        int DrawFrame();

        int maxFramesInFlight = 0;
        int currentFrame = 0;

        bool* framebufferResizedFlag = nullptr;
};

#endif