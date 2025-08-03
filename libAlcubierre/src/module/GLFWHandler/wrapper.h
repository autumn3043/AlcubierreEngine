#ifndef ALCENGINE_MODULE_GLFWHANDLER_WRAPPER_H
#define ALCENGINE_MODULE_GLFWHANDLER_WRAPPER_H

#include "core/Registry/wrapper.h"
#include "module/GLFWHandler/public.h"

//Extra notes cuz this is the template one
class GLFWWrapper : public WrapperBaseClass{
    static ModuleRegisterBundle bundle { []() -> WrapperBaseClass* { return new GLFWWrapper(); }}; //Statically pass pointer to constructor (as WrapperBaseClass*()) to registry

    GLFWHandler* handler; //When the wrapper is created, it is owned by the registry and held in an unmap of WrapperBaseClasss. It then constructs and owns its native implementation.

    GLFWWrapper() {
        handler = new GLFWHandler();
        Registry::GetRegistry().RegisterService(handler->IWindowManager_IMPL);
    }
};

#endif