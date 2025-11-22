#include "module/VulkanHandler/public.h"

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
    DebugManager::Punchcard bootstrapTimer;

    environment = new VulkanEnvironmentComponent(this, registry_ptr);
    device = new VulkanDeviceComponent(this, registry_ptr);

    if(!dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER))->Get<bool>("defer-renderchain-initialisation", false)) {
        swapchain = new VulkanSwapchainComponent(this, registry_ptr);
        renderchain = new VulkanRenderchainComponent(this, registry_ptr);
    }

    DM().Log("Finished graphics backend bootstrapping in " + std::to_string(bootstrapTimer.delta()) + " milliseconds. Awaiting frame draw command");
}

#include "module/VulkanHandler/Component/Environment/source.inl"
#include "module/VulkanHandler/Component/Device/source.inl"
#include "module/VulkanHandler/Component/Swapchain/source.inl"
#include "module/VulkanHandler/Component/Renderchain/source.inl"

void VulkanHandler::drawFrameImpl() {
    renderchain->DrawFrame();
}