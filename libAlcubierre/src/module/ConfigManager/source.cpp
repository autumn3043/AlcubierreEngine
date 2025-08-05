#include "module/ConfigManager/public.h"
#include "module/ConfigManager/private.h"

#include <cstring>

DebugManager& DM = DebugManager::GetDebugManager();

std::string to_string (nlohmann::json::value_t t) {
    static constexpr const char* names[] = {
        "null", "object", "array", "string", "boolean",
        "number_integer", "number_unsigned", "number_float",
        "binary", "discarded"
    };
    std::size_t idx = static_cast<std::size_t>(t);
    return idx < std::size(names) ? names[idx] : "unknown";
}

ConfigManager::ConfigManager() : IConfigManager_ConfigManager(this) {
    PrivatePtr = new ConfigManagerImpl();
}

ConfigManager::~ConfigManager() {
    delete PrivatePtr;
}

int ConfigManager::Get_Impl(Container& v_out) {
    if(PrivatePtr->RawConfig.contains(v_out.key)) {
        if(v_out.T_info == GetJsonTypeId(RawConfig[v_out.key])) {
            v_out.ptr = std::move(ExtractPtrFromJson(RawConfig[v_out.key])); //This must be the ONLY assignment of the pointer, and ONLY invoked if the assigning type is the same as the requested type, which is what the deletion logic assumes.
            return 0;

        } else {
            DM.Log("Failed to retrieve key '" + key + "' because its type did not match the requested type");
            return 1;
        }

    } else {
        DM.Log("Failed to retrieve key '" + key + "' because it does not exist in memory"); 
        return 1;
    }
}

int ConfigManager::Set_Impl(const std::string& key, const void* value) {
    if(!PrivatePtr->RawConfig.contains(key)) {
        DM.Log("Assignment error: cannot inspect type of key '" + key + "' because it does not exist");
        return 1;

    } else {
        nlohmann::json value_cast = PrivatePtr->CastNativeToJson(PrivatePtr->RawConfig[key], value);
        if(!value_cast.is_null()) return PrivatePtr->LoadItemToMemory(key, value_cast); //LoadItemToMemory moves value_cast so its ok for it go out of scope once it finishes
        else return 1;
    } 
}

int ConfigManager::Import_Impl(const std::string& dataset) { 
    nlohmann::json parsedData;

    try {
        parsedData = nlohmann::json::parse(dataset.c_str());

    } catch (nlohmann::json::parse_error E) {
        DM.Log(E);
        return 1;

    } catch (...) {
        throw;
    }

    return PrivatePtr->LoadObjectToMemory(parsedData); 
}

ConfigManagerImpl::ConfigManagerImpl() {
    RawConfig = nlohmann::json();
}

int ConfigManagerImpl::LoadItemToMemory(const std::string& key, nlohmann::json& item) {
    if(!RawConfig.contains(key)) {
        RawConfig.push_back(std::move(item));
        return 0;

    } else if (RawConfig[key].type() == item.type()) {
        RawConfig[key] = std::move(item);
        return 0;

    } else {
        DM.Log("Assignment error: key '" + key + "' has type " + to_string(RawConfig[key].type()) + " but was trying to assign a value of type " + to_string(item));
        return 1;
    }
}

int ConfigManagerImpl::LoadObjectToMemory(nlohmann::json object) {
    for(auto& [key, val] : object.items()) { 
    //nlohmann asserts that the type here is auto for some reason instead of whatever the underlying type is (which I cannot even find in the docs) so we have this bullshit. 
    //I can only assume this exists purely to torture me, the #1 certified auto hater.
        LoadItemToMemory(key, val);
    }

    return 0;
}

bool ConfigManagerImpl::CompareType(const nlohmann::json& json) {
    switch(json.type()) {
        case json::value_t::boolean:
            return typeid(bool);

        case json::value_t::number_integer:
            return typeid(int64_t);

        case json::value_t::number_unsigned:
            return typeid(uint64_t);

        case json::value_t::number_float:
            return typeid(double);

        case json::value_t::string:
            return typeid(std::string);

        case json::value_t::array:
            return typeid();

        case json::value_t::binary:
            return typeid();

        case json::value_t::object:
            return typeid(nlohmann::json);

        default:
            return typeid(void); 
    }
}

const void* ConfigManagerImpl::ExtractPtrFromJson(const nlohmann::json& json) {
    void* Rvalue = nullptr;

    switch(json.type()) {
        case nlohmann::json::value_t::boolean:
            Rvalue = new bool(json.get<bool>());
        break;

        case nlohmann::json::value_t::number_integer:
            Rvalue = new int64_t(json.get<int64_t>());
        break;

        case nlohmann::json::value_t::number_unsigned:
            Rvalue = new uint64_t(json.get<uint64_t>());
        break;

        case nlohmann::json::value_t::number_float:
            Rvalue = new double(json.get<double>());
        break;

        case nlohmann::json::value_t::string:
            Rvalue = new std::string(json.get<std::string>());
        break;

        case nlohmann::json::value_t::array:
            std::vector<void*> hold = new std::vector<void*>(); //A local array (non-pointer) of the JSON elements (pointers.)

            for(const nlohmann::json& item : json) {
                hold.push_back(ExtractPtrFromJson(item)); //Fill that array out with support for recursion.
            }

            Rvalue = static_cast<void*>(hold); //Create the return pointer by passing the pointer we already created.
        break;

        case nlohmann::json::value_t::binary:
            Rvalue = new nlohmann::json::binary_t(json.get<nlohmann::json::binary_t>());
        break;

        case nlohmann::json::value_t::object:
            Rvalue = new nlohmann::json(json);
        break;

    }

    return Rvalue;
}

nlohmann::json ConfigManagerImpl::CastNativeToJson(nlohmann::json& target, const void* value) {
    try {
        nlohmann::json hold;

        switch(target.type()) {
            case nlohmann::json::value_t::boolean:
                hold = *static_cast<bool*>(value);

            case nlohmann::json::value_t::number_integer:
                hold = *static_cast<int64_t*>(value);

            case nlohmann::json::value_t::number_unsigned:
                hold = *static_cast<uint64_t*>(value);

            case nlohmann::json::value_t::number_float:
                hold = *static_cast<double*>(value);

            case nlohmann::json::value_t::string:
                hold = *static_cast<std::string*>(value);

            case nlohmann::json::value_t::array:
                hold = *static_cast<const nlohmann::json::array_t*>(value);

            case nlohmann::json::value_t::binary:
                hold = *static_cast<const nlohmann::json::binary_t*>(value);

            case nlohmann::json::value_t::object:
                hold = *static_cast<const nlohmann::json::object_t*>(value);

            default:
                DM.Log("Assignment error: key '" + "' cannot be modified because it holds an unrecognised type");
                return 1;
        }
        return hold;

    } catch (const nlohmann::json::type_error& E) {
        DM.Log("Assignment error: key '" + key + "' has type " + to_string(target.type()) + " but nlohmann indicated that the type of the provided void* was different: " + E.message);
        return nlohmann::json();

    } catch (...) {
        throw;
    }
}