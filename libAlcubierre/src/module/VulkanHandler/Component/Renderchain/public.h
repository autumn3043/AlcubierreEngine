#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H

class VulkanRenderchainComponent {
    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanRenderchainComponent();

        VkPipeline Pipeline = VK_NULL_HANDLE;
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkCommandPool CommandPool = VK_NULL_HANDLE;
            int max_frames_in_flight = 2;
            int currentFrame = 0;

        bool* framebufferResizedFlag;

    private:
        class RenderFrame {
            public:
                const VkDevice& device;
                RenderFrame(VkDevice& device);
                ~RenderFrame();

                void CreateSemaphores();
                
                VkCommandBuffer commandBuffer;

                VkSemaphore semaphore = VK_NULL_HANDLE;
                VkFence fence = VK_NULL_HANDLE;
        };
        std::vector<RenderFrame> renderFrames;

        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;
        
        const uint64_t t_second = 1000000000; //qty of nanoseconds in 1 second
        const uint64_t TIMEOUT_SET = 5; //seconds
        
        int CreateGraphicsPipeline();
            int CreateShader(VkShaderModule& shader);
            void FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule);
            VkShaderStageFlagBits ConfigParse_ShaderStage(std::string& value);

        int CreateCommandPool();
            void GetCommandPoolCreateInfo(AlcCommandPoolCreateInfo& ReturnBundle);
        int CreateCommandBuffers();
            void GetCommandBufferCreateInfo(AlcCommandBufferCreateInfo& ReturnBundle);
        int CreateVertexBuffers();
            struct Vertex {
                float position[2];
                float colour[3];
            };
            class VertexBuffer {
                public:
                    VertexBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, std::vector<Vertex> vertices);
                    ~VertexBuffer();

                    int fillBufferMemory(void** external_membuffer);

                    VkDevice& device;

                    VkDeviceMemory bufferMemory;
                    VkBuffer bufferInstance;
                    VkDeviceSize bufferSize;
            };
            std::vector<VertexBuffer*> vertexBuffers;
            std::vector<Vertex> vertices_temp;
            void* vertexData;

        public: int DrawFrame();
        private:
            int RecreateSwapchain();
            int RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image);            
            
};

#endif