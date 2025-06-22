#include "AlcubierreEngine.h"

AlcubierreEngine::AlcubierreEngine() {
    try {
        Init();
    } catch (DebugManager::AlcExcept exception) {
        DebugManager::Log(exception);
    }
}

int AlcubierreEngine::Init() {
    GLFW = GLFWHandler();
    VK = VulkanHandler();

    return 0;
}