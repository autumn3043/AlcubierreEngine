#include "core/PublicInterface/private.h"
#include "core/PublicInterface/public.h"

#include "core/DebugManager/public.h"

AlcubierreEngine::AlcubierreEngine() {
    PrivatePtr = new AlcubierreEngineImpl();
}

AlcubierreEngine::~AlcubierreEngine() {
    delete PrivatePtr;
}

void AlcubierreEngine::initEngine() { return PrivatePtr->initEngine(); }

void AlcubierreEngine::Debug::log(std::string message, int priority) { return PrivatePtr->log(message, priority); }

int AlcubierreEngine::Config::get(std::vector<std::string> key, bool* returnPtr) { return PrivatePtr->get<bool>(key, returnPtr); }
int AlcubierreEngine::Config::get(std::vector<std::string> key, int* returnPtr) { return PrivatePtr->get<int>(key, returnPtr); }
int AlcubierreEngine::Config::get(std::vector<std::string> key, float* returnPtr) { return PrivatePtr->get<float>(key, returnPtr); }
int AlcubierreEngine::Config::get(std::vector<std::string> key, std::string* returnPtr) { return PrivatePtr->get<std::string>(key, returnPtr); }
int AlcubierreEngine::Config::parse(std::string value) { return PrivatePtr->parse(value); }
int AlcubierreEngine::Config::dump() {return PrivatePtr->dump(); }

bool AlcubierreEngine::Window::shouldClose() { return PrivatePtr->shouldClose(); }

int AlcubierreEngine::Graphics::frame() { return PrivatePtr->frame(); }

AlcubierreEngineImpl::AlcubierreEngineImpl() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registryMasterInstance.FetchService(CONFIGURATION_MANAGER));
    CM->Set<std::string>({"protected", "metadata", "engine_data", "name"}, "\"Alcubierre Engine\"");
    std::string engineVersion_str = "[" + std::to_string(ENGINE_VERSION[0]) + "," + std::to_string(ENGINE_VERSION[1]) + "," + std::to_string(ENGINE_VERSION[2]) + "]";
    CM->Set<std::vector<int>>({"protected", "metadata", "engine_data", "version"}, engineVersion_str);
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {}

void AlcubierreEngineImpl::log(std::string& message, int& priority) {
    std::string appName = dynamic_cast<IConfigManager*>(registryMasterInstance.FetchService(CONFIGURATION_MANAGER))->Get<std::string>({"metadata", "application_data", "name"}, "Application");
    DM().Log(DebugReport(message, priority, appName));
}

int AlcubierreEngineImpl::parse(std::string& value) {
    return dynamic_cast<IConfigManager*>(registryMasterInstance.FetchService(CONFIGURATION_MANAGER))->SetRaw(value);
}
int AlcubierreEngineImpl::dump() {
    return dynamic_cast<IConfigManager*>(registryMasterInstance.FetchService(CONFIGURATION_MANAGER))->dump();
}

void AlcubierreEngineImpl::initEngine() {
    registryMasterInstance.FetchService(WINDOW_SURFACE); //Must come before Vulkan so it can dump to cfg. (Part of registry pre init hook TODO)
    registryMasterInstance.FetchService(GRAPHICS_BACKEND);
}

bool AlcubierreEngineImpl::shouldClose() {
    return dynamic_cast<IWindowManager*>(registryMasterInstance.FetchService(WINDOW_MANAGER))->shouldClose();
}

int AlcubierreEngineImpl::frame() {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(registryMasterInstance.FetchService(WINDOW_MANAGER));
    WM->pollEvents();
    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registryMasterInstance.FetchService(GRAPHICS_BACKEND));
    GB->drawFrame();
    return 0;
}