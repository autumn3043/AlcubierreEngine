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
            void FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule);

        int CreateCommandPool();
            void GetCommandPoolCreateInfo(AlcCommandPoolCreateInfo& ReturnBundle);
        int CreateCommandBuffers();
            void GetCommandBufferCreateInfo(AlcCommandBufferCreateInfo& ReturnBundle);

        public: int DrawFrame();
        private:
            int RecreateSwapchain();
            int RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image);            
            
};

#endif