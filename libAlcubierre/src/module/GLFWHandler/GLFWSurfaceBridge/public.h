#ifndef ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PUBLIC_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IWindowManager.h"
#include "core/Registry/interface/IConfigManager.h"
//Provides
#include "core/Registry/interface/IWindowSurfaceBridge.h"

class GLFWSurfaceBridgeImpl;

class GLFWSurfaceBridge : public WrapperBaseClass{
    public:
        Registry* registry_ptr = nullptr;
        GLFWSurfaceBridge(void* registry);
        ~GLFWSurfaceBridge();

        GLFWSurfaceBridgeImpl* PrivatePtr = nullptr;
        
        int CreateWindowSurfaceImpl(void* TargetInstance, void* TargetSurfaceObject);

        class IWindowSurfaceBridgeImpl : public IWindowSurfaceBridge {
            public:
                GLFWSurfaceBridge* Parent;

                IWindowSurfaceBridgeImpl(GLFWSurfaceBridge* _parent) : Parent(_parent) {}
                int CreateWindowSurface(void* TargetInstance, void* TargetSurfaceObject) override { return Parent->CreateWindowSurfaceImpl(TargetInstance, TargetSurfaceObject); }
        };

        IWindowSurfaceBridgeImpl IWindowSurfaceBridge_GLFWSurfaceBridge;

        static ModuleRegistryBundle bundle;
};

#endif