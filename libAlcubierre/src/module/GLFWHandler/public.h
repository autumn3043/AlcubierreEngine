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

        void* GetWindowObjectImpl();

        class IWindowManagerImpl : public IWindowManager {
            public:
                GLFWHandler* Parent;

                IWindowManagerImpl(GLFWHandler* _parent) : Parent(_parent) {}

                void* GetWindowObject() override { return Parent->GetWindowObjectImpl(); }
        };

        IWindowManagerImpl IWindowManager_GLFWHandler;
        
        GLFWImpl* PrivatePtr;
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