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
                void* GetWindowObject() override { return Parent->GetWindowObjectImpl(); }
                WindowInfo* GetWindowInfo() override { return Parent->GetWindowInfoImpl(); }
                bool TouchSurfaceApi() override { return Parent->TouchSurfaceApiImpl(); }
                bool ShouldClose() override { return Parent->ShouldCloseImpl(); }
                void pollEvents() override { return Parent->pollEventsImpl(); }
                bool* getFramebufferResizedFlag() override { return Parent->getFramebufferResizedFlagImpl(); }
        };

        IWindowManagerImpl IWindowManager_GLFWHandler;

        void* GetWindowObjectImpl();
        IWindowManager::WindowInfo* GetWindowInfoImpl();
        bool TouchSurfaceApiImpl();
        bool ShouldCloseImpl();
        void pollEventsImpl();
        bool suppressFirstResize();
        bool* getFramebufferResizedFlagImpl();
    
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

        GLFWwindow* Window;

        int Init();
        int CreateWindowImpl();
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        bool apiStatus = false;
        IWindowManager::WindowInfo* WindowInfo = nullptr;
        bool framebufferResizedFlag = false;
        bool suppressFirstResizeFlag = true;
};

#endif