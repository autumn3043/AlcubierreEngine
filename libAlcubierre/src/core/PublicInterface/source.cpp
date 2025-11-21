#include "core/PublicInterface/private.h"
#include "core/PublicInterface/public.h"

#include "core/DebugManager/public.h"

AlcubierreEngine::AlcubierreEngine() {
    PrivatePtr = new AlcubierreEngineImpl();
}

AlcubierreEngine::~AlcubierreEngine() {
    delete PrivatePtr;
}

bool AlcubierreEngine::ShouldClose() {
    return PrivatePtr->ShouldCloseImpl();
}

void AlcubierreEngine::Frame() {
    return PrivatePtr->FrameImpl();
}

AlcubierreEngineImpl::AlcubierreEngineImpl() {
    registry.FetchService(WINDOW_SURFACE); //Must come before Vulkan so it can dump to cfg. (Part of registry pre init hook TODO)
    registry.FetchService(GRAPHICS_BACKEND);
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
}

bool AlcubierreEngineImpl::ShouldCloseImpl() {
    return dynamic_cast<IWindowManager*>(registry.FetchService(WINDOW_MANAGER))->ShouldClose();
}

int i = 0;

void AlcubierreEngineImpl::FrameImpl() {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(registry.FetchService(WINDOW_MANAGER));
    WM->pollEvents();
    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry.FetchService(GRAPHICS_BACKEND));
    GB->drawFrame();
}