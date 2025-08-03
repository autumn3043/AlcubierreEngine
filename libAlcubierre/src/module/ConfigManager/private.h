#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H

#include "core/DebugManager/wrapper.h"
using DM = DebugManager::GetDebugManager();

class ConfigManagerException : public AlcEngineException {
    public:
        ConfigManagerException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

#include <nlohmann/json.hpp>
#include <utility>
#include <memory>
#include <vector>
#include <type_traits>

class ConfigManagerIMPL {
    nlohmann::json json;

    int Import(const char* userdata, const char* appdata);

    template <typename T>
    T GetFromJson(const nlohmann::json& json, const std::string& key, bool R, int I);
};

#endif