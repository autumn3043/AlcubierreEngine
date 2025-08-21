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

        nlohmann::json RawConfig;

        IConfigManager::TypeDescriptor GetDescriptorFromJson(const nlohmann::json& json);
        void* GetPointerToJson(const nlohmann::json& json);
        void SetFromParsed(const nlohmann::json& json);
        
};

#endif