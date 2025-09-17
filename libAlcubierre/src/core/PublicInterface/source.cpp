#include "core/PublicInterface/private.h"
#include "core/PublicInterface/public.h"

AlcubierreEngine::AlcubierreEngine() {
    PrivatePtr = new AlcubierreEngineImpl();
}

AlcubierreEngine::~AlcubierreEngine() {
    delete PrivatePtr;
}

AlcubierreEngineImpl::AlcubierreEngineImpl() {
    Registry::GetRegistry().Init();

    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    CM->Set<std::vector<std::string>>("extensions", "[\"VK_EXT_debug_utils\"]");
    CM->Set<int>("window_width", "900");

    //Graphics queue
    CM->Set<std::vector<int>>({"required_graphics_queues", "0", "flags"}, "[1]");
    CM->Set<int>({"required_graphics_queues", "0", "count"}, "1");
    CM->Set<int>({"required_graphics_queues", "0", "priority"}, "1");

    //Surface queue
    CM->Set<bool>({"required_graphics_queues", "1", "surface_support"}, "true");
    CM->Set<int>({"required_graphics_queues", "1", "count"}, "1");
    CM->Set<int>({"required_graphics_queues", "1", "priority"}, "1");

    Registry::GetRegistry().FetchService("IWindowSurfaceBridge"); //Must come before Vulkan so it can dump to cfg.
    Registry::GetRegistry().FetchService("IGraphicsBackend");
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
}
