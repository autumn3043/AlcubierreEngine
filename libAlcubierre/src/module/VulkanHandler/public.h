#ifndef ALCENGINE_MODULE_VULKANHANDLER_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_PUBLIC_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IConfigManager.h"
#include "core/Registry/interface/IWindowSurfaceBridge.h"

//Provides
#include "core/Registry/interface/IGraphicsBackend.h"

#include "core/DebugManager/public.h"

class VulkanException : public AlcEngineException {
    public:
        VulkanException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class VulkanHandlerIMPL;

class VulkanHandler {
    public:
        VulkanHandler();
        ~VulkanHandler();

        void* GetBackendObjectImpl();

        class IGraphicsBackendImpl : public IGraphicsBackend {
            public:
                VulkanHandler* Parent;

                IGraphicsBackendImpl(VulkanHandler* _parent) : Parent(_parent) {}

                void* GetBackendObject() override { return Parent->GetBackendObjectImpl(); }
        };

        IGraphicsBackendImpl IGraphicsBackend_VulkanHandler;

        VulkanHandlerIMPL* PrivatePtr;
};

class VulkanHandlerWrapper : public WrapperBaseClass{
    public:
        VulkanHandlerWrapper() {
            native = new VulkanHandler();
            Registry::GetRegistry().RegisterService(native->IGraphicsBackend_VulkanHandler);
        }

        ~VulkanHandlerWrapper() {
            delete native;
        }

    private:
        VulkanHandler* native = nullptr;

        static ModuleRegistryBundle bundle;
};

#endif 