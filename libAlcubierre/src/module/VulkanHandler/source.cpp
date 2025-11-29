#include "module/VulkanHandler/public.h"
#define NULL_BIT 0x0

ModuleRegistryBundle VulkanHandler::bundle(
    [](void* registry) -> WrapperBaseClass* { return new VulkanHandler(registry); },
    {GRAPHICS_BACKEND},
    {WINDOW_SURFACE}, //Swapchain images point to window surface elements which must remain valid during shutdown to avoid a segmentation fault
    "VulkanHandler"
);

VulkanHandler::VulkanHandler(void* registry) 
    :   IGraphicsBackend_VulkanHandler(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{GRAPHICS_BACKEND, &IGraphicsBackend_VulkanHandler}};
        Init();
    }

VulkanHandler::~VulkanHandler() {
    if(device) {
        if(device->Device != VK_NULL_HANDLE) vkDeviceWaitIdle(device->Device);
    }

    if(renderchain) delete renderchain;
    if(swapchain) delete swapchain;
    if(device) delete device;
    if(environment) delete environment;
}

void VulkanHandler::Init() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    DebugManager::Punchcard bootstrapTimer;

    CM->Set<int>({"protected", "metadata", "api_version"}, std::to_string(API_VERSION));
    CM->Set<int>({"protected", "graphics", "default_presentation_mode"}, std::to_string(VK_PRESENT_MODE_FIFO_KHR));

    environment = new VulkanEnvironmentComponent(this, registry_ptr);
    device = new VulkanDeviceComponent(this, registry_ptr);

    if(!CM->Get<bool>({"renderer", "defer_renderchain_initialisation"}, false)) {
        swapchain = new VulkanSwapchainComponent(this, registry_ptr);
        renderchain = new VulkanRenderchainComponent(this, registry_ptr);
        chainInitialisation = true;
    } else {
        DM().Log("Deferring renderchain initialisation to first frame draw call", 1);
    }

    DM().Log("Finished graphics backend init in " + std::to_string(bootstrapTimer.delta()) + " milliseconds. Awaiting frame draw command", 1);
}

//We want the components to be included in THIS translation unit
#include "module/VulkanHandler/Component/Environment/source.inl"
#include "module/VulkanHandler/Component/Device/source.inl"
#include "module/VulkanHandler/Component/Swapchain/source.inl"
#include "module/VulkanHandler/Component/Renderchain/source.inl"

void VulkanHandler::drawFrameImpl() {
    if(!chainInitialisation) {
        DM().Log("Proceeding with deferred intialisation", 1);
        swapchain = new VulkanSwapchainComponent(this, registry_ptr);
        renderchain = new VulkanRenderchainComponent(this, registry_ptr);
        chainInitialisation = true;
    }

    renderchain->DrawFrame();
}

void VulkanHandler::recreateSwapchain() {
    DM().Log("Recreating Vulkan swapchain");
    vkDeviceWaitIdle(device->Device);
    if(swapchain) delete swapchain;
    swapchain = new VulkanSwapchainComponent(this, registry_ptr);
}

#undef NULL_BIT