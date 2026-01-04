#ifndef ALCENGINE_MODULE_VULKANHANDLER_H
#define ALCENGINE_MODULE_VULKANHANDLER_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IConfigManager.h"
#include "core/Registry/interface/IWindowSurfaceBridge.h"
#include "core/Registry/interface/IWindowManager.h"
//Provides
#include "core/Registry/interface/IGraphicsBackend.h"

#include "core/DebugManager/public.h"

class VulkanException : public AlcEngineException {
    public:
        VulkanException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

#include <vulkan/vulkan.h>
#include "module/VulkanHandler/VulkanStructBundles.h"

class VulkanHandler;
#include "module/VulkanHandler/Component/Environment/public.h"
#include "module/VulkanHandler/Component/Device/public.h"
#include "module/VulkanHandler/Component/Allocator/public.h"
#include "module/VulkanHandler/Component/Swapchain/public.h"
#include "module/VulkanHandler/Component/Renderchain/public.h"

class VulkanHandler : public WrapperBaseClass {
    public:
        VulkanHandler(void* registry);
        ~VulkanHandler();

        VulkanEnvironmentComponent* environment = nullptr;
        VulkanDeviceComponent* device = nullptr;
        bool chainInitialisation = false;
        VulkanSwapchainComponent* swapchain = nullptr;
        VulkanRenderchainComponent* renderchain = nullptr;
        VulkanMemoryAllocatorComponent* allocator = nullptr;

        void Init();

        class IGraphicsBackendImpl : public IGraphicsBackend {
            public:
                VulkanHandler* Parent;

                IGraphicsBackendImpl(VulkanHandler* _parent) : Parent(_parent) {}

                int createObjectBuffer(uint32_t& modelHash, std::vector<Vector>& vertices, std::vector<uint32_t>& indices) override { return Parent->createObjectBuffer(modelHash, vertices, indices); }
                int clearFrame() override { return Parent->clearFrame(); }
                int addObjectToFrame(uint32_t& modelHash, Vector& position) override { return Parent->addObjectToFrame(modelHash, position); }
                int drawFrame() override { return Parent->drawFrame(); }
        };

        IGraphicsBackendImpl IGraphicsBackend_VulkanHandler;

        int createObjectBuffer(uint32_t& modelHash, std::vector<Vector>& vertices, std::vector<uint32_t>& indices);
        int clearFrame();
        int addObjectToFrame(uint32_t& modelHash, Vector& position);
        int drawFrame();

        void recreateSwapchain();

    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;
        
        const uint32_t API_VERSION = VK_API_VERSION_1_4;
};

#endif