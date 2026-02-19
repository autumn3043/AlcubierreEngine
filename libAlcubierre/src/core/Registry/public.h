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
    GRAPHICS_BACKEND = 4,
    MODEL_LOADER = 5,
    DIRECTOR = 6,
    HASH_GENERATOR = 7,
    DISK_INTERFACE = 8,
    SHADER_LOADER = 9
};

inline std::string ServiceID_str(ServiceID id) {
    switch (id) {
        case DEBUG_MESSENGER:
            return "DEBUG_MESSENGER";
        break;
        case CONFIGURATION_MANAGER:
            return "CONFIGURATION_MANAGER";
        break;
        case WINDOW_MANAGER:
            return "WINDOW_MANAGER";
        break;
        case WINDOW_SURFACE:
            return "WINDOW_SURFACE";
        break;
        case GRAPHICS_BACKEND:
            return "GRAPHICS_BACKEND";
        break;
        case MODEL_LOADER:
            return "MODEL_LOADER";
        break;
        case DIRECTOR:
            return "DIRECTOR";
        break;
        case HASH_GENERATOR:
            return "HASH_GENERATOR";
        break;
        case DISK_INTERFACE:
            return "DISK_INTERFACE";
        break;
        case SHADER_LOADER:
            return "SHADER_LOADER";
        break;
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