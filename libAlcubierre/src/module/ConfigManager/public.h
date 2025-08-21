#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H

#include "core/Registry/public.h"

//Services
#include "core/Registry/interface/IConfigManager.h"

class ConfigManagerImpl;

class ConfigManager {
    public:
        int Get_Impl(IConfigManager::Container& v_out);
        int Set_Impl(IConfigManager::Container& v_in);

        class IConfigManagerImpl : public IConfigManager {
            public:
                ConfigManager* Parent;

                IConfigManagerImpl(ConfigManager* _parent) : Parent(_parent) {}

                int GetInternal(IConfigManager::Container& v_out) override { return Parent->Get_Impl(v_out); }
                int SetInternal(IConfigManager::Container& v_in) override { return Parent->Set_Impl(v_in); }
        };

        ConfigManager();
        ~ConfigManager();

        IConfigManagerImpl IConfigManager_ConfigManager;
        
    private:
        ConfigManagerImpl* PrivatePtr;
};

class ConfigManagerWrapper : public WrapperBaseClass{
    public:
        ConfigManagerWrapper() {
            native = new ConfigManager();
            Registry::GetRegistry().RegisterService(native->IConfigManager_ConfigManager);
        }

        ~ConfigManagerWrapper() {
            delete native;
        }

    private:
        ConfigManager* native = nullptr;

        static ModuleRegistryBundle bundle;
};

#endif