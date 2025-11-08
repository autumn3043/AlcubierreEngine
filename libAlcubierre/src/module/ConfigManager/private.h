#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H

#include "core/DebugManager/public.h"

class ConfigManagerException : public AlcEngineException {
    public:
        ConfigManagerException(std::string message) : AlcEngineException(DebugReport(message, 0, "ConfigManager")) {}
};

#include <nlohmann/json.hpp>
#include <string>

class ConfigManagerImpl {
    public:
        Registry* registry_ptr = nullptr;
        ConfigManagerImpl(Registry* registry);
        ~ConfigManagerImpl();

        nlohmann::json RawConfig;

        IConfigManager::TypeDescriptor GetDescriptorFromJson(const nlohmann::json& json);
        void* GetPointerToJson(const nlohmann::json& json);
        int get_impl(IConfigManager::Container& v_out);
        int set_impl(IConfigManager::Container& v_in);

    private:
        void loc_Log(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "ConfigManager"), Write); }
        std::string fullkey(const std::vector<std::string>& key);
        
};

#endif