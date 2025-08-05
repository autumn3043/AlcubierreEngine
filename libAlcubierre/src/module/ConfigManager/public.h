#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H

#include <string>
#include "core/Registry/interface/IConfigManager.h"

class ConfigManagerImpl;

class ConfigManager {
    public:
        int Get_Impl(Container& v_out);
        int Set_Impl(const std::string& key, const void* value);
        int Import_Impl(const std::string& dataset);

        class IConfigManagerImpl : public IConfigManager {
            public:
                ConfigManager* Parent;

                IConfigManagerImpl(ConfigManager* _parent) : Parent(_parent) {}

                int Get(const std::string& key, Container& v_out) override { return Parent->Get_Impl(v_out); }
                int Set(const std::string& key, const void* value) override { return Parent->Set_Impl(key, value); }
                int Import(const std::string& dataset) override { return Parent->Import_Impl(dataset); }
        };

        ConfigManager();
        ~ConfigManager();

        IConfigManagerImpl IConfigManager_ConfigManager;
        
    private:
        ConfigManagerImpl* PrivatePtr;
};

#endif