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

    dynamic_cast<IWindowManager*>(Registry::GetRegistry().FetchService("IWindowManager"))->GetWindowObject();
    dynamic_cast<IGraphicsBackend*>(Registry::GetRegistry().FetchService("IGraphicsBackend"))->GetBackendObject();
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
}