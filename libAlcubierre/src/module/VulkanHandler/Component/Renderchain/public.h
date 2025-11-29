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

                const VkDevice& device;
                
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

        int CreateCommandPool();
            class CommandPool;

            struct RenderFrame {
                const VkDevice& device;

                RenderFrame(VkDevice& _device, VkCommandPool& commandPool, int _timeout);
                ~RenderFrame();

                const int timeout;
                
                VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
                VkSemaphore semaphore = VK_NULL_HANDLE;
                VkFence fence = VK_NULL_HANDLE;
            };

            class CommandPool {
                public:
                    CommandPool(VkDevice& _device, int queueIndex, int _maxFramesInFlight, int frameTimeoutSeconds);
                    ~CommandPool();

                    std::vector<RenderFrame> renderFrames;

                private:
                    VkDevice& device;

                    int maxFramesInFlight;
                    int frameTimeout;

                    VkCommandPool commandPool = VK_NULL_HANDLE;

            };
        CommandPool* commandPool = nullptr;

        int CreateVertexBuffers();
            struct Vertex {
                float position[2];
                float colour[3];
            };
            class VertexBuffer {
                public:
                    VertexBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, std::vector<Vertex> vertices);
                    ~VertexBuffer();

                    int fillBufferMemory(std::vector<Vertex>& external_membuffer);

                    const VkDevice& device;

                    VkDeviceMemory bufferMemory;
                    VkBuffer bufferInstance;
                    VkDeviceSize bufferSize;
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