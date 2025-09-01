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
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
    
}