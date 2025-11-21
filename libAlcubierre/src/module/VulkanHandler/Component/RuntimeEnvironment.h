#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RUNTIMEENVIRONMENT_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_RUNTIMEENVIRONMENT_H

class VulkanRuntimeEnvironmentComponent {
    public:
        VulkanRuntimeEnvironmentComponent(VulkanHandlerIMPL* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
            CreateVulkanInstance();
            CreateDebugLink();
            CreateSurface();
        };

        ~VulkanRuntimeEnvironmentComponent();

        VkInstance& getInstance() { return Instance; }
        VkSurfaceKHR& getSurface() { return Surface; }

    private:
        VulkanHandlerIMPL* parent = nullptr;
        Registry*& registry_ptr;

        VkInstance Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR Surface = VK_NULL_HANDLE;

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