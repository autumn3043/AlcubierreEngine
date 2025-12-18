#ifndef ALCENGINE_MODULE_GLFWHANDLER_H
#define ALCENGINE_MODULE_GLFWHANDLER_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IConfigManager.h"
//Provides
#include "core/Registry/interface/IWindowManager.h"

#include "core/DebugManager/public.h"

class GLFWException : public AlcEngineException {
    public:
        GLFWException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

#include <GLFW/glfw3.h>

class GLFWHandler : public WrapperBaseClass{
    public:
        GLFWHandler(void* registry);
        ~GLFWHandler();

        class IWindowManagerImpl : public IWindowManager {
            public:
                GLFWHandler* Parent;

                IWindowManagerImpl(GLFWHandler* _parent) : Parent(_parent) {}
                void* getWindowObject() override { return Parent->getWindowObject(); }
                WindowInfo getWindowInfo() override { return Parent->getWindowInfo(); }
                void pollEvents() override { return Parent->pollEvents(); }
                bool shouldClose() override { return Parent->shouldClose(); }
        };

        IWindowManagerImpl IWindowManager_GLFWHandler;

        void* getWindowObject();
        IWindowManager::WindowInfo getWindowInfo();
        void pollEvents();
        bool shouldClose();
    
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

        GLFWwindow* Window;

        int init();
        int createWindow();
};

#endif