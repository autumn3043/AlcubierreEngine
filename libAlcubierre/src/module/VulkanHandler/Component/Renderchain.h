#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_H

class VulkanRenderchainComponent {
    public:
        VulkanRenderchainComponent(VulkanHandlerIMPL* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
            CreateGraphicsPipeline();
            CreateCommandPool();
            CreateCommandBuffers();

            AllocateSemaphores(semaphores_presentComplete, 5);
            AllocateSemaphores(semaphores_renderFinished, 5);
            AllocateFences(fences_draw, 2);
        };

        ~VulkanRenderchainComponent();

        VkCommandBuffer& getCommandBuffer(int index) { return CommandBuffers[index]; }
        VkSemaphore& getPresentationSemaphore(int index) { return semaphores_presentComplete[index]; }
        VkSemaphore& getRenderSemaphore(int index) { return semaphores_renderFinished[index]; }
        VkFence& getDrawingFence(int index) { return fences_draw[index]; }

        int RecordCommandBuffer_(VkCommandBuffer& CommandBuffer, int imageIndex) { return RecordCommandBuffer(CommandBuffer, imageIndex); }

    private:
        VulkanHandlerIMPL* parent = nullptr;
        Registry*& registry_ptr;

        VkPipeline Pipeline = VK_NULL_HANDLE;
        VkCommandPool CommandPool = VK_NULL_HANDLE;
            std::vector<VkCommandBuffer> CommandBuffers;
            int max_frames_in_flight = 2;
            int currentFrame = 0;
        std::vector<VkSemaphore> semaphores_presentComplete;
        std::vector<VkSemaphore> semaphores_renderFinished;
        std::vector<VkFence> fences_draw;
        

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