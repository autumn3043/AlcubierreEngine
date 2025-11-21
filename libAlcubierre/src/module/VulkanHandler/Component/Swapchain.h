#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_SWAPCHAIN_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_SWAPCHAIN_H

class VulkanSwapchainComponent {
    public:
        VulkanSwapchainComponent(VulkanHandlerIMPL* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
            CreateSwapchain();
        };

        ~VulkanSwapchainComponent();

        VkSwapchainKHR& getSwapchain() { return Swapchain; }
        VkImage& getChainImage(int index) { return ChainImages[index]; }
        VkImageView& getChainImageView(int index) { return ChainImageViews[index]; }
        VkExtent2D& getSwapExtent() { return frameExtent; }
        VkFormat& getSwapFormat() { return ChainFormat; }

    private:
        VulkanHandlerIMPL* parent = nullptr;
        Registry*& registry_ptr;

        uint32_t frameWidth;
        uint32_t frameHeight;
        VkExtent2D frameExtent;

        VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
            std::vector<VkImage> ChainImages;
            std::vector<VkImageView> ChainImageViews;
            VkFormat ChainFormat;

        int CreateSwapchain();
            int FillSwapchainImages(VkSwapchainCreateInfoKHR& ChainInitInfo);
            void FetchSwapMode(VkPresentModeKHR& ReturnMode);
            void FetchSwapSurfaceFormat(VkFormat& ReturnFormat, VkColorSpaceKHR& ReturnColor);
};

#endif