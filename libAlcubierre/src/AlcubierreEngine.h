#ifndef ALCUBIERRE_ENGINE_ALE_H
#define ALCUBIERRE_ENGINE_ALE_H

#include "module/VulkanHandler/VulkanHandler.h"
#include "module/GLFWHandler/GLFWHandler.h"
#include "module/DebugManager/DebugManager.h"

#include <memory>

class AlcubierreEngine {
    public:
        AlcubierreEngine();
    
    private:
        std::unique_ptr<VulkanHandler> VK;
        std::unique_ptr<GLFWHandler> GLFW;

        int Init();
        void Cleanup();
};

#endif