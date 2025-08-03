#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H

#include <string>
#include "core/Registry/interface/IConfigManager.h"

class IConfigManagerImpl : public IConfigManager {

};

class ConfigManagerIMPL;

class ConfigManager {
    public:
        ConfigManager(const char* userdata = nullptr, const char* appdata = nullptr);
        ~ConfigManager();

        IConfigManagerImpl IConfigManager_IMPL;
        
    private:
        ConfigManagerIMPL* PrivatePtr;
};

#endif