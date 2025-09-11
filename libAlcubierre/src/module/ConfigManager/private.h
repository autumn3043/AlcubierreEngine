#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H

#include "core/DebugManager/public.h"

class ConfigManagerException : public AlcEngineException {
    public:
        ConfigManagerException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

#include <nlohmann/json.hpp>
#include <string>

class ConfigManagerImpl {
    public:
        ConfigManagerImpl();
        ~ConfigManagerImpl();

        nlohmann::json RawConfig;

        IConfigManager::TypeDescriptor GetDescriptorFromJson(const nlohmann::json& json);
        void* GetPointerToJson(const nlohmann::json& json);
        int get_impl(IConfigManager::Container& v_out);
        int set_impl(IConfigManager::Container& v_in);
        
};

#endif