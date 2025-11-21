#ifndef ALCENGINE_MODULE_VULKANHANDLER_PRIVATE_H
#define ALCENGINE_MODULE_VULKANHANDLER_PRIVATE_H

#include <vulkan/vulkan.h>
#include "module/VulkanHandler/VulkanStructBundles.h"

#include "module/VulkanHandler/Component/RuntimeEnvironment.h"
#include "module/VulkanHandler/Component/Device.h"
#include "module/VulkanHandler/Component/Swapchain.h"
#include "module/VulkanHandler/Component/Renderchain.h"

class VulkanHandlerIMPL {
    public:
        VulkanHandlerIMPL(Registry* registry);
        ~VulkanHandlerIMPL();

        VulkanRuntimeEnvironmentComponent* environment = nullptr;
        VulkanDeviceComponent* device = nullptr;
        VulkanSwapchainComponent* swapchain = nullptr;
        VulkanRenderchainComponent* renderchain = nullptr;

        void drawFrame();

    private:
        Registry* registry_ptr = nullptr;
};

#endif