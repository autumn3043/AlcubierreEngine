#ifndef ALCENGINE_MODULE_VULKANHANDLER_H
#define ALCENGINE_MODULE_VULKANHANDLER_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IConfigManager.h"
#include "core/Registry/interface/IWindowSurfaceBridge.h"
#include "core/Registry/interface/IWindowManager.h"
#include "core/Registry/interface/IShaders.h"
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
#include "module/VulkanHandler/Component/Pipelines/public.h"
#include "module/VulkanHandler/Component/Renderchain/public.h"

class VulkanHandler : public WrapperBaseClass {
    public:
        VulkanHandler(void* registry);
        ~VulkanHandler();

        VulkanEnvironmentComponent* environment = nullptr;
        VulkanDeviceComponent* device = nullptr;
        VulkanDeviceComponent::QueueHandle graphicsRenderingQueue;
        VulkanMemoryAllocatorComponent* allocator = nullptr;
        VulkanSwapchainComponent* swapchain = nullptr;
        VulkanPipelineComponent* pipelines = nullptr;
        VulkanRenderchainComponent* renderchain = nullptr;
        DebugManager::Punchcard timeToFirstLive;

        void Init();

        class IGraphicsBackendImpl : public IGraphicsBackend {
            public:
                VulkanHandler* Parent;

                IGraphicsBackendImpl(VulkanHandler* _parent) : Parent(_parent) {}

                int storeMesh(Hash_T hash, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices) override { return Parent->allocator->storeMesh(hash, vertices, indices); }
                int discardMesh(Hash_T hash) override { return Parent->allocator->discardMesh(hash); }

                int storeTexture();
                int discardTexture();

                int addToFrame(SceneObject object) override { return Parent->renderchain->addToFrame(object); }
                int clearFrame() override { return Parent->renderchain->clearFrame(); }
                int drawFrame() override { return Parent->renderchain->drawFrame(); }
        };

        IGraphicsBackendImpl IGraphicsBackend_VulkanHandler;

        void recreateSwapchain();

    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;
        
        const uint32_t API_VERSION = VK_API_VERSION_1_4;
};

#define NULL_BIT 0x0

#endif