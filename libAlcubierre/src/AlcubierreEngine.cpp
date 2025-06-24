#include "AlcubierreEngine.h"

AlcubierreEngine::AlcubierreEngine() {
    try {
        Init();
    } catch (AlcExceptions::AlcExcept& _E) {
        DebugManager::Log(_E);
    }

    Cleanup();
}

int AlcubierreEngine::Init() {
    GLFW = std::make_unique<GLFWHandler>();
    VK = std::make_unique<VulkanHandler>();

    return 0;
}

void AlcubierreEngine::Cleanup() {
}