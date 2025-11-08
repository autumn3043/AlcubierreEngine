#ifndef ALCENGINE_CORE_REGISTRY_PUBLIC_H
#define ALCENGINE_CORE_REGISTRY_PUBLIC_H

#include <functional>
#include <string>

#include "core/Registry/base/WrapperBaseClass.h"
#include "core/Registry/base/InterfaceBaseClass.h"

enum ServiceID {
    DEBUG_MESSENGER = 0,
    CONFIGURATION_MANAGER = 1,
    WINDOW_MANAGER = 2,
    WINDOW_SURFACE = 3,
    GRAPHICS_BACKEND = 4
};

inline std::string ServiceID_str(ServiceID id) {
    switch (id) {
        case DEBUG_MESSENGER:
            return "DEBUG_MESSENGER";
        case CONFIGURATION_MANAGER:
            return "CONFIGURATION_MANAGER";
        case WINDOW_MANAGER:
            return "WINDOW_MANAGER";
        case WINDOW_SURFACE:
            return "WINDOW_SURFACE";
        case GRAPHICS_BACKEND:
            return "GRAPHICS_BACKEND";
        default:
            return "UNKNOWN_SERVICE_ID";
    }
}

struct ModuleRegistryBundle {
    std::function<WrapperBaseClass*(void*)> Constructor;

    std::string Label;

    std::vector<ServiceID> Provides;
    std::vector<ServiceID> Depends;

    ModuleRegistryBundle(std::function<WrapperBaseClass*(void*)> _constructor, std::vector<ServiceID> _provides, std::vector<ServiceID> _depends, std::string _label = "UNSET_MODULE_LABEL");
};

class RegistryImpl;

class Registry {
    public:
        static std::vector<ModuleRegistryBundle*>& moduleQueueStaticAccessor() {
            static std::vector<ModuleRegistryBundle*> queue;
            return queue;
        }

        InterfaceBaseClass*& FetchService(ServiceID id);

        Registry();
        ~Registry();

    private:
        RegistryImpl* PrivatePtr = nullptr;
};

#endif