#ifndef ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IConfigManager.h"
//Provides
#include "core/Registry/interface/IWindowManager.h"

class GLFWImpl;

class GLFWHandler : public WrapperBaseClass{
    public:
        Registry* registry_ptr = nullptr;
        GLFWHandler(void* registry);
        ~GLFWHandler();

        void* GetWindowObjectImpl();
        IWindowManager::WindowInfo* GetWindowInfoImpl();
        bool TouchSurfaceApiImpl();

        class IWindowManagerImpl : public IWindowManager {
            public:
                GLFWHandler* Parent;

                IWindowManagerImpl(GLFWHandler* _parent) : Parent(_parent) {}
                void* GetWindowObject() override { return Parent->GetWindowObjectImpl(); }
                IWindowManager::WindowInfo* GetWindowInfo() override { return Parent->GetWindowInfoImpl(); }
                bool TouchSurfaceApi() override { return Parent->TouchSurfaceApiImpl(); }
        };

        IWindowManagerImpl IWindowManager_GLFWHandler;
        
        GLFWImpl* PrivatePtr = nullptr;
    
    private:
        static ModuleRegistryBundle bundle;
};

#endif