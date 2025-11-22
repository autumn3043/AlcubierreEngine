#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ENVIRONMENT_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_ENVIRONMENT_PUBLIC_H

class VulkanEnvironmentComponent {
    public:
        VulkanEnvironmentComponent(VulkanHandler* _parent, Registry* _registry_ptr);
        ~VulkanEnvironmentComponent();

        VkInstance Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR Surface = VK_NULL_HANDLE;

    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

        int CreateVulkanInstance();
            void FetchCreateData(AlcInstanceCreateInfo& ReturnBundle);
            void FetchAppData(AlcApplicationInfo& ReturnBundle);

        int CreateDebugLink();
            void FetchDebugData(AlcDebugUtilsMessengerCreateInfoEXT& ReturnBundle);

            static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

        int CreateSurface();
};

#endif