#include "module/VulkanHandler/private.h"
        
static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "VulkanHandler"), Write); }

//We want the components to be included in THIS translation unit
#include "module/VulkanHandler/Component/Environment/source.inl"
#include "module/VulkanHandler/Component/Device/source.inl"
#include "module/VulkanHandler/Component/Allocator/source.inl"
#include "module/VulkanHandler/Component/Swapchain/source.inl"
#include "module/VulkanHandler/Component/Pipelines/source.inl"
#include "module/VulkanHandler/Component/Renderchain/source.inl"


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
    if(pipelines) delete pipelines;
    if(swapchain) delete swapchain;
    if(allocator) delete allocator;
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
    graphicsRenderingQueue = device->fetchQueueHandle(VulkanDeviceComponent::queueType::GRAPHICS);
    allocator = new VulkanMemoryAllocatorComponent(this, registry_ptr);
    swapchain = new VulkanSwapchainComponent(this, registry_ptr);
    pipelines = new VulkanPipelineComponent(this, registry_ptr);
    renderchain = new VulkanRenderchainComponent(this, registry_ptr);

    logIdentity("Finished graphics backend init in " + std::to_string(bootstrapTimer.delta()) + " milliseconds. Awaiting frame draw command", 1);
    allocator->dumpMeshMemoryLayout();

    timeToFirstLive.punch();
}

void VulkanHandler::recreateSwapchain() {
    logIdentity("Recreating Vulkan swapchain");
    vkDeviceWaitIdle(device->Device);
    if(swapchain) delete swapchain;
    swapchain = new VulkanSwapchainComponent(this, registry_ptr);
}

#undef NULL_BIT