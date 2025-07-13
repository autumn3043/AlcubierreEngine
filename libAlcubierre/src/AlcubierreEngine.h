#ifndef ALCUBIERRE_ENGINE_ALE_H
#define ALCUBIERRE_ENGINE_ALE_H

#include "module/VulkanHandler/VulkanHandler.h"
#include "module/GLFWHandler/GLFWHandler.h"
#include "core/AlcubierreCore.h"

#include <memory>

class AlcubierreEngine {
    public:
        struct AlcubierreInitInfo {
            const char* AppConfig = nullptr;
            const char* UserConfig = nullptr;
        };

        AlcubierreEngine(AlcubierreInitInfo& InitInfo);
        ~AlcubierreEngine();
    
    private:
        std::unique_ptr<VulkanHandler> VK;
        std::unique_ptr<GLFWHandler> GLFW;

        int Init(AlcubierreInitInfo& InitInfo);
        void Cleanup();
};

#endif