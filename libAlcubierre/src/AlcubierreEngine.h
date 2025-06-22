#ifndef ALCUBIERRE_ENGINE_ALE_H
#define ALCUBIERRE_ENGINE_ALE_H

#include "module/VulkanHandler/VulkanHandler.h"
#include "module/GLFWHandler/GLFWHandler.h"
#include "module/DebugManager/DebugManager.h"

class AlcubierreEngine {
    public:
        AlcubierreEngine();

        void DebugLog(std::string);
        void DebugLog(std::exception);
    
    private:
        int Init();

        VulkanHandler VK;
        GLFWHandler GLFW;
};

#endif