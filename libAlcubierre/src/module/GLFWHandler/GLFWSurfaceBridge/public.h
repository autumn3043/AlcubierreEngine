#ifndef ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PUBLIC_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IWindowManager.h"
//Provides
#include "core/Registry/interface/IWindowSurfaceBridge.h"

class GLFWSurfaceBridge {
    public:
        int CreateWindowSurfaceImpl(void* TargetInstance, void* TargetSurfaceObject);

        class IWindowSurfaceBridgeImpl : public IWindowSurfaceBridge {
            public:
                GLFWSurfaceBridge* Parent;

                IWindowSurfaceBridgeImpl(GLFWSurfaceBridge* _parent) : Parent(_parent) {}

                int CreateWindowSurface(void* TargetInstance, void* TargetSurfaceObject) override { return Parent->CreateWindowSurfaceImpl(TargetInstance, TargetSurfaceObject); }
        };

        GLFWSurfaceBridge();
        ~GLFWSurfaceBridge();

        IWindowSurfaceBridgeImpl IWindowSurfaceBridge_GLFWSurfaceBridge;
};

class GLFWSurfaceBridgeWrapper : public WrapperBaseClass{
    public:
        GLFWSurfaceBridgeWrapper() {
            native = new GLFWSurfaceBridge();
            Registry::GetRegistry().RegisterService(native->IWindowSurfaceBridge_GLFWSurfaceBridge);
        }

        ~GLFWSurfaceBridgeWrapper() {
            delete native;
        }

    private:
        GLFWSurfaceBridge* native = nullptr;

        static ModuleRegistryBundle bundle;
};

#endif