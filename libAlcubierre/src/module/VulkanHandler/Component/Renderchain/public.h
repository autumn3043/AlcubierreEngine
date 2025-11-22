#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H

class VulkanRenderchainComponent {
    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanRenderchainComponent();

        VkPipeline Pipeline = VK_NULL_HANDLE;
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkCommandPool CommandPool = VK_NULL_HANDLE;
            std::vector<VkCommandBuffer> CommandBuffers;
            int max_frames_in_flight = 2;
            int currentFrame = 0;
        std::vector<VkSemaphore> semaphores_presentComplete;
        std::vector<VkSemaphore> semaphores_renderFinished;
        std::vector<VkFence> fences_draw;

        void DrawFrame();

    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;
        

        int CreateGraphicsPipeline();
            void FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule);

        int CreateCommandPool();
            void GetCommandPoolCreateInfo(AlcCommandPoolCreateInfo& ReturnBundle);

        int CreateCommandBuffers();
            void GetCommandBufferCreateInfo(AlcCommandBufferCreateInfo& ReturnBundle);

        int RecordCommandBuffer(VkCommandBuffer& CommandBuffer, int imageIndex);
            struct AlcImageLayoutDetails{
                VkPipelineStageFlags2 stageMask;
                VkAccessFlags2 accessMask;
                VkImageLayout layout;
                uint32_t queueIndex;

                AlcImageLayoutDetails(VkPipelineStageFlags2 _stageMask, VkAccessFlags2 _accessMask, VkImageLayout _layout, uint32_t _queueIndex = VK_QUEUE_FAMILY_IGNORED)
                    :   stageMask(_stageMask),
                        accessMask(_accessMask),
                        layout(_layout),
                        queueIndex(_queueIndex)
                    {}
            };
            
            int TransitionImageLayout(VkCommandBuffer& CommandBuffer, VkImage& _image, AlcImageLayoutDetails& _oldDetails, AlcImageLayoutDetails& _newDetails);

        int AllocateSemaphores(std::vector<VkSemaphore>& semaphores, int count);
        int AllocateFences(std::vector<VkFence>& fences, int count);
};

#endif