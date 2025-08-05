#ifndef ALCENGINE_MODULE_CONFIGMANAGER_WRAPPER_H
#define ALCENGINE_MODULE_CONFIGMANAGER_WRAPPER_H

#include "core/Registry/wrapper.h"
#include "module/ConfigManager/public.h"

class ConfigManagerWrapper : public WrapperBaseClass{
    static ModuleRegisterBundle bundle { []() -> WrapperBaseClass* { return new ConfigManagerWrapper(); }};

    ConfigManager* native;

    ConfigManagerWrapper() {
        native = new ConfigManager();
        Registry::GetRegistry().RegisterService(native->IConfigManager_ConfigManager);
    }

    ~ConfigManagerWrapper() {
        delete native;
    }
};

#endif