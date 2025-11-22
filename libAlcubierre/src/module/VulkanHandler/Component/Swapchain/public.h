#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_SWAPCHAIN_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_SWAPCHAIN_PUBLIC_H

class VulkanSwapchainComponent {
    public:
        VulkanSwapchainComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanSwapchainComponent();

        uint32_t frameWidth;
        uint32_t frameHeight;
        VkExtent2D frameExtent;

        VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
            std::vector<VkImage> ChainImages;
            std::vector<VkImageView> ChainImageViews;
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