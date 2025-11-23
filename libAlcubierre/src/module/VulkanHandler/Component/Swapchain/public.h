#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_SWAPCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_SWAPCHAIN_PUBLIC_H

class VulkanSwapchainComponent {
    public:
        VulkanSwapchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanSwapchainComponent();

        uint32_t frameWidth;
        uint32_t frameHeight;
        VkExtent2D frameExtent;

        enum LAYOUT_DETAILS_PRESET {
            LAYOUT_DETAILS_PRESET_UNDEFINED = 0,
            LAYOUT_DETAILS_PRESET_COLOURATTACHMENT = 1,
            LAYOUT_DETAILS_PRESET_PRESENT = 2
        };
        static std::vector<AlcImageLayoutDetails> imageLayoutPresets;
        
        class SwapchainImageWrapper {
            public:
                const VkDevice& device;
                const VkExtent2D extent;

                SwapchainImageWrapper(VkDevice& _device, VkExtent2D _extent);
                ~SwapchainImageWrapper();

                int TransitionImageLayout(VkCommandBuffer& CommandBuffer, AlcImageLayoutDetails& layoutDetails_target);

                VkImage imageHandle = VK_NULL_HANDLE;
                VkImageView imageView = VK_NULL_HANDLE;

                VkSemaphore semaphore = VK_NULL_HANDLE;

                AlcImageLayoutDetails layoutDetails;
        };

        VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
            std::vector<SwapchainImageWrapper> Images;
            VkFormat ChainFormat;

    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

        int CreateSwapchain();
            int FillSwapchainImages(VkSwapchainCreateInfoKHR& ChainInitInfo);
            void FetchSwapMode(VkPresentModeKHR& ReturnMode);
            void FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor);
};

#endif