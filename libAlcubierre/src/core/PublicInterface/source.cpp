#include "core/PublicInterface/private.h"
#include "core/PublicInterface/public.h"

#include "core/DebugManager/public.h"

AlcubierreEngine::AlcubierreEngine() {
    PrivatePtr = new AlcubierreEngineImpl();
}

AlcubierreEngine::~AlcubierreEngine() {
    delete PrivatePtr;
}

void AlcubierreEngine::InitEngine() {
    return PrivatePtr->InitEngineImpl();
}

bool AlcubierreEngine::ShouldClose() {
    return PrivatePtr->ShouldCloseImpl();
}

void AlcubierreEngine::Frame() {
    return PrivatePtr->FrameImpl();
}

int AlcubierreEngine::SetConfigFromJsonString(std::string jsonString) {
    return PrivatePtr->SetConfigFromJsonStringImpl(jsonString);
}

AlcubierreEngineImpl::AlcubierreEngineImpl() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr.FetchService(CONFIGURATION_MANAGER));
    CM->Set<std::string>({"protected", "metadata", "engine_data", "name"}, "\"Alcubierre Engine\"");
    std::string engineVersion_str = "[" + std::to_string(ENGINE_VERSION[0]) + "," + std::to_string(ENGINE_VERSION[1]) + "," + std::to_string(ENGINE_VERSION[2]) + "]";
    CM->Set<std::vector<int>>({"protected", "metadata", "engine_data", "version"}, engineVersion_str);
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
}

void AlcubierreEngineImpl::InitEngineImpl() {
    registry_ptr.FetchService(WINDOW_SURFACE); //Must come before Vulkan so it can dump to cfg. (Part of registry pre init hook TODO)
    registry_ptr.FetchService(GRAPHICS_BACKEND);

    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr.FetchService(GRAPHICS_BACKEND));
}

bool AlcubierreEngineImpl::ShouldCloseImpl() {
    return dynamic_cast<IWindowManager*>(registry_ptr.FetchService(WINDOW_MANAGER))->ShouldClose();
}

void AlcubierreEngineImpl::FrameImpl() {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(registry_ptr.FetchService(WINDOW_MANAGER));
    WM->pollEvents();
    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr.FetchService(GRAPHICS_BACKEND));
    GB->drawFrame();
}

int AlcubierreEngineImpl::SetConfigFromJsonStringImpl(std::string jsonString) {
    return dynamic_cast<IConfigManager*>(registry_ptr.FetchService(CONFIGURATION_MANAGER))->SetRaw(jsonString);
}