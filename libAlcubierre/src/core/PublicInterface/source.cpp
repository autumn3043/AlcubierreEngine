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
    Registry::GetRegistry().Init();

    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    CM->Set<std::vector<std::string>>("extensions", {"[\"VK_EXT_debug_utils\"]"});
    CM->Set<int>("window_width", "900");
    
    dynamic_cast<IWindowManager*>(Registry::GetRegistry().FetchService("IWindowManager"))->GetWindowObject();
    dynamic_cast<IGraphicsBackend*>(Registry::GetRegistry().FetchService("IGraphicsBackend"))->GetBackendObject();
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
}
