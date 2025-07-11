#include "AlcubierreEngine.h"

AlcubierreEngine::AlcubierreEngine(AlcubierreInitInfo& InitInfo) {
    try {
        Init(InitInfo);
    } catch (AlcExceptions::AlcExcept& _E) {
        DebugManager::Log(_E);
    }
}

AlcubierreEngine::~AlcubierreEngine() {
    Cleanup();
}

int AlcubierreEngine::Init(AlcubierreInitInfo& InitInfo) {
    DataManager::GetDataManager(InitInfo.UserConfig, InitInfo.AppConfig);
    GLFW = std::make_unique<GLFWHandler>();
    VK = std::make_unique<VulkanHandler>();

    return 0;
}

void AlcubierreEngine::Cleanup() {
    VK.reset();
    GLFW.reset();
}