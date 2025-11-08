#include "core/PublicInterface/private.h"
#include "core/PublicInterface/public.h"

#include "core/DebugManager/public.h"

AlcubierreEngine::AlcubierreEngine() {
    PrivatePtr = new AlcubierreEngineImpl();
}

AlcubierreEngine::~AlcubierreEngine() {
    delete PrivatePtr;
}

AlcubierreEngineImpl::AlcubierreEngineImpl() {
    Registry registry = Registry();

    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry.FetchService(CONFIGURATION_MANAGER));

    CM->Set<bool>("debug", "true");

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

    registry.FetchService(WINDOW_SURFACE); //Must come before Vulkan so it can dump to cfg.
    registry.FetchService(GRAPHICS_BACKEND);
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
}