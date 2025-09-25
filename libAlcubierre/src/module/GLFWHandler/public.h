#ifndef ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IConfigManager.h"
//Provides
#include "core/Registry/interface/IWindowManager.h"

class GLFWImpl;

class GLFWHandler {
    public:
        GLFWHandler();
        ~GLFWHandler();

        int WakeImpl();
        void* GetWindowObjectImpl();
        IWindowManager::WindowInfo* GetWindowInfoImpl();

        class IWindowManagerImpl : public IWindowManager {
            public:
                GLFWHandler* Parent;

                IWindowManagerImpl(GLFWHandler* _parent) : Parent(_parent) {}

                int Wake() override { return Parent->WakeImpl(); }
                void* GetWindowObject() override { return Parent->GetWindowObjectImpl(); }
                IWindowManager::WindowInfo* GetWindowInfo() override { return Parent->GetWindowInfoImpl(); }
        };

        IWindowManagerImpl IWindowManager_GLFWHandler;
        
        GLFWImpl* PrivatePtr = nullptr;
};

class GLFWHandlerWrapper : public WrapperBaseClass{
    public:
        GLFWHandlerWrapper() {
            native = new GLFWHandler();
            Registry::GetRegistry().RegisterService(native->IWindowManager_GLFWHandler);
        }

        ~GLFWHandlerWrapper() {
            delete native;
        }

    private:
        GLFWHandler* native = nullptr;

        static ModuleRegistryBundle bundle;
};

#endif