#ifndef ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PUBLIC_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IWindowManager.h"
#include "core/Registry/interface/IConfigManager.h"
//Provides
#include "core/Registry/interface/IWindowSurfaceBridge.h"

#include "core/DebugManager/public.h"

class GLFWSurfaceBridge : public WrapperBaseClass{
    public:
        GLFWSurfaceBridge(void* registry);
        ~GLFWSurfaceBridge();
        
        int createWindowSurface(void* TargetInstance, void* TargetSurfaceObject);

        class IWindowSurfaceBridgeImpl : public IWindowSurfaceBridge {
            public:
                GLFWSurfaceBridge* Parent;

                IWindowSurfaceBridgeImpl(GLFWSurfaceBridge* _parent) : Parent(_parent) {}
                int createWindowSurface(void* TargetInstance, void* TargetSurfaceObject) override { return Parent->createWindowSurface(TargetInstance, TargetSurfaceObject); }
        };

        IWindowSurfaceBridgeImpl IWindowSurfaceBridge_GLFWSurfaceBridge;

    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

        int init();
};

#endif