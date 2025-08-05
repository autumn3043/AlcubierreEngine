#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PRIVATE_H

#include "core/DebugManager/wrapper.h"

class ConfigManagerException : public AlcEngineException {
    public:
        ConfigManagerException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

#include <nlohmann/json.hpp>
#include <string>

class ConfigManagerImpl {
    public:
        int LoadItemToMemory(const std::string& key, nlohmann::json& item);
        int LoadObjectToMemory(nlohmann::json object);

        ConfigManagerImpl();

        nlohmann::json RawConfig;

        nlohmann::json CastNativeToJson(nlohmann::json& target, const void* value);
        const void* ExtractPtrFromJson(const nlohmann::json& json);

        template <typename T>
        const typeinfo& RebuildArrayStructure(const nlohmann::json& json) {
            switch(json.type()) {
                case json::value_t::boolean:
                    return typeid(std::vector<bool>);

                case json::value_t::number_integer:
                    return typeid(std::vector<int64_t>);

                case json::value_t::number_unsigned:
                    return typeid(std::vector<uint64_t>);

                case json::value_t::number_float:
                    return typeid(std::vector<double>);

                case json::value_t::string:
                    return typeid(std::vector<std::string>);

                case json::value_t::array:
                    if(json.get<std::vector<nlohmann::json>>().empty()) {
                        return std::vector<nlohmann::json::null_t>();
                    } else {
                        return RebuildArrayStructure(json.get<std::vector<nlohmann::json>>().front());
                    }

                case json::value_t::binary:
                    return typeid(std::vector<nlohmann::json::binary_t>);

                case json::value_t::object:
                    return typeid(std::vector<nlohmann::json>);

                default:
                    return typeid(std::vector<nlohmann::json::null_t>); 
            }
        }
};

#endif