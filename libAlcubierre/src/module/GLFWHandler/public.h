#ifndef ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H

#include "core/Registry/public.h"

//Services
#include "core/Registry/interface/IWindowManager.h"

class GLFWImpl;

class GLFWHandler {
    public:
        GLFWHandler();
        ~GLFWHandler();

    private:
        GLFWImpl* PrivatePtr;
};

class GLFWHandlerWrapper : public WrapperBaseClass{
    public:
        GLFWHandlerWrapper() {
            native = new GLFWHandler();
            // Registry::GetRegistry().RegisterService(native->IWindowManager_GLFWHandler);
        }

        ~GLFWHandlerWrapper() {
            delete native;
        }

    private:
        GLFWHandler* native = nullptr;

        static ModuleRegistryBundle bundle;
};

#endif