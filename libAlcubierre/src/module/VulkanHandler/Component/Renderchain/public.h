#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RENDERCHAIN_PUBLIC_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

class VulkanRenderchainComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

    public:
        VulkanRenderchainComponent(VulkanHandler* _parent, Registry*& _registry_ptr);
        ~VulkanRenderchainComponent();

    private:
        VulkanDeviceComponent::QueueHandle* graphicalQueue;
        VulkanDeviceComponent::QueueHandle presentationQueue;

        int createFrames();

    public:
        int addToFrame(SceneObject object);
        int clearFrame();

        int drawFrame();

    private:
        struct meshSet {
            VkBuffer* vertexBuffer;
            VkBuffer* indexBuffer;
            std::vector<SceneObject> objects;

            meshSet(VkBuffer* _vertexBuffer, VkBuffer* _indexBuffer) : vertexBuffer(_vertexBuffer), indexBuffer(_indexBuffer), objects({}) {}
        };
    
        std::unordered_map<uint32_t, meshSet> sceneTree;

    private:
        VkCommandPool renderingCommandPool = VK_NULL_HANDLE;
        VkDescriptorPool descriptorSetPool = VK_NULL_HANDLE;
        VkQueryPool queryPool = VK_NULL_HANDLE;

        char* frameDataHostBufferAccessIndex;
        VkDeviceMemory frameDataHostMemory = VK_NULL_HANDLE;
        VkBuffer frameDataHostBuffer = VK_NULL_HANDLE;
        VkDeviceMemory frameDataDeviceMemory = VK_NULL_HANDLE;
        VkBuffer frameDataDeviceBuffer = VK_NULL_HANDLE;

        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkDescriptorSet> descriptorSets;

        class Frame {
            public:
                Frame(VkDevice& _device, uint64_t _frameIndex);
                ~Frame();

            private:
                VkDevice& device;

            public:
                struct FrameData {
                    glm::mat4 view;
                    glm::mat4 projection;
                };

                uint64_t frameIndex;

                //Synchronisation
                VkFence fence_frameInFlight = VK_NULL_HANDLE;
                VkSemaphore semaphore_awaitImageGrab = VK_NULL_HANDLE;
                uint32_t queryPoolBaseIndex;
        };

        std::vector<Frame> frames;
        int maxFramesInFlight = 0;
        int commandTimeout = INT_MAX;
        int currentFrameIndex = 0;
        
        std::vector<float> frameDeltas;
        int FRAMEDELTACACHESIZE;
        uint64_t framesEver = 0;

    public:
        int updateUniformBuffer(char* buffer, VulkanSwapchainComponent::SwapchainImageWrapper* image);
        int recordCommandBuffer(Frame* frame, VulkanSwapchainComponent::SwapchainImageWrapper* image);
};

#endif