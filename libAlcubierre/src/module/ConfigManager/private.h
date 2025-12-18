#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H

#include "core/Registry/public.h"

//Services
#include "core/Registry/interface/IConfigManager.h"

#include "core/DebugManager/public.h"

class ConfigManagerException : public AlcEngineException {
    public:
        ConfigManagerException(std::string message) : AlcEngineException(DebugReport(message, 0, "ConfigManager")) {}
};

#include <nlohmann/json.hpp>

class ConfigManager : public WrapperBaseClass{
    public:
        int get(IConfigManager::Container& v_out);
        int set(IConfigManager::Container& v_in);
        int setParse(IConfigManager::Container& v_in);

        class IConfigManagerImpl : public IConfigManager {
            public:
                ConfigManager* Parent;

                IConfigManagerImpl(ConfigManager* _parent) : Parent(_parent) {}

                int getInternal(IConfigManager::Container& v_out) override { return Parent->get(v_out); }
                int setInternal(IConfigManager::Container& v_in) override { return Parent->set(v_in); }
                int setParseInternal(IConfigManager::Container& v_in) override { return Parent->setParse(v_in); }
        };

        ConfigManager(void* registry);
        ~ConfigManager();

        IConfigManagerImpl IConfigManager_ConfigManager;
        
    private:
        Registry* registry_ptr = nullptr;
        static ModuleRegistryBundle bundle;

        int init();

        nlohmann::json RawConfig;

        IConfigManager::TypeDescriptor getDescriptorFromJson(const nlohmann::json& json);
        void* getPointerToJson(const nlohmann::json& json);

        void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "ConfigManager"), Write); }
        std::string fullkey(const std::vector<std::string>& key);
        void popEmptyElements(nlohmann::json& json);
};

#endif