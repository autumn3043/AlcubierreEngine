#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H

class VulkanRenderchainComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry*& _registry_ptr);
        ~VulkanRenderchainComponent();

    public:
        int addToFrame(SceneObject* object);
        int clearFrame();

        int drawFrame();

    private:
        struct MeshSet {
            VulkanMemoryAllocatorComponent::bufferSetHandle set;
            std::vector<SceneObject*> objects;
        };
    
        std::unordered_map<uint32_t, MeshSet> sceneTree;

        int recordCommandBuffer(VkCommandBuffer& commandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image);

    private:
        struct CommandBuffer {
            VkDevice& device;

            CommandBuffer(VkDevice& _device, VkCommandPool& commandPool, int _timeout);
            ~CommandBuffer();

            const int timeout;
            
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            VkSemaphore semaphore = VK_NULL_HANDLE;
            VkFence fence = VK_NULL_HANDLE;
        };

        std::vector<CommandBuffer*> commandBuffers;
        VkCommandPool commandPool = VK_NULL_HANDLE;

        int maxFramesInFlight;
        int currentFrame;
        int commandTimeout;
};

#endif