#include "module/ConfigManager/private.h"

#include <cstring>

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "ConfigManager"), Write); }

ModuleRegistryBundle ConfigManager::bundle(
    [](void* registry) -> WrapperBaseClass* { return new ConfigManager(registry); },
    {CONFIGURATION_MANAGER},
    {},
    "ConfigManager"
);

ConfigManager::ConfigManager(void* registry) 
    :   IConfigManager_ConfigManager(this),
        registry_ptr(static_cast<Registry*>(registry))
    {   
        Services.insert({CONFIGURATION_MANAGER, &IConfigManager_ConfigManager});
        init();
    }

ConfigManager::~ConfigManager() {}

int ConfigManager::init() {
    RawConfig = nlohmann::json::object();
    return 0;
}

int ConfigManager::get(IConfigManager::Container& v_out) {
    //Does a value exist at this key?
    nlohmann::json* json = &RawConfig;
    for(int i = 0; i < v_out.key.size(); i++) {
        if(v_out.key[i] == CFGARRAY_SIZE_T) {
            if(json->is_primitive()) {
                logIdentity("Stored value at key '" + fullkey(v_out.key) + "' was: " + getDescriptorFromJson(*json).Type() + " which is not an array or object type", 1);
                return 1;
            } else {
                v_out.ptr = new int(json->size());
                return 0;
            }
        } else if(json->is_array()) {
            int arrayKey;
            try {
                arrayKey = std::stoi(v_out.key[i]);
                json = &((*json)[arrayKey]);
            } catch (std::invalid_argument& exception) {
                std::vector<std::string> thisKey = v_out.key;
                thisKey.erase(thisKey.begin() + i, thisKey.end());
                logIdentity("The value at key '" + fullkey(thisKey) + "' is an array but the subsequent keynotch was '" + v_out.key[i] + "' which is not a numeric array index", 1);
            }
        } else if(!json->contains(v_out.key[i])) {
            logIdentity("Failed to get value at key '" + fullkey(v_out.key) +"' because it did not exist", 0);
            return 1;
        } else json = &((*json)[v_out.key[i]]);
    }

    //Does its type match the expected return type?
    if((*v_out.t_info != getDescriptorFromJson(*json))) {
        logIdentity("Stored value at key '" + fullkey(v_out.key) + "' was: " + getDescriptorFromJson(*json).Type() + " which did not match the requested type: " + v_out.t_info->Type(), 1);
        return 2;
    }

    //If yes to both, get, return and move a pointer to it.
    v_out.ptr = std::move(getPointerToJson(*json));
    return 0;
}

int ConfigManager::set(IConfigManager::Container& v_in) {
    std::string input = *static_cast<std::string*>(v_in.ptr);
    nlohmann::json parsed;

    try {
        parsed = nlohmann::json::parse(input);
    } catch (const nlohmann::json::parse_error& E) {
        logIdentity("Failed to parse string: '" + input + "'\nVerbose error: " + E.what(), 2);
        return 1;
    } 

    try {
        nlohmann::json* json = &RawConfig;

        for(int i = 0; i < v_in.key.size(); i++) {
            const std::string& notch = v_in.key[i];

            if(i + 1 < v_in.key.size()) {
                if(!json->contains(notch)) {
                    (*json)[notch] = nlohmann::json::object();
                }
                json = &((*json)[notch]);

            } else {
                if(!json->contains(notch)) {
                    (*json)[notch] = parsed;
                    return 0;
                }

                if((*json)[notch].is_array() && (*json)[notch].empty() && parsed.is_array()) {
                    (*json)[notch] = parsed;
                    return 0;
                }
                
                if(*v_in.t_info == getDescriptorFromJson((*json)[notch])) {
                    (*json)[notch].swap(parsed);
                    return 0;
                } 

                logIdentity("Failed to insert key '" + fullkey(v_in.key) + "' of type: " + v_in.t_info->Type() + " due to mismatch with stored type: " + getDescriptorFromJson((*json)[notch]).Type(), 1);
                return 1;
            }
        }
    } catch (const std::exception& E) {
        logIdentity("Failed to insert key '" + fullkey(v_in.key) + "' of type: " + v_in.t_info->Type() + "\nVerbose error: " + E.what() + "\n" + RawConfig.dump(), 2);
        return 1;
    }

    return -1;
}

int ConfigManager::setParse(IConfigManager::Container& v_in) {
    std::string value = *static_cast<std::string*>(v_in.ptr);
    nlohmann::json parsed;

    try {
        parsed = nlohmann::json::parse(value, nullptr, true, true);
    } catch (const nlohmann::json::parse_error& E) {
        logIdentity("Failed to parse string: '" + value + "'\nVerbose error: " + E.what(), 2);
        return 1;
    }

    if(parsed.contains("protected")) parsed.erase("protected");
    if(parsed.empty()) return 1;
    if(parsed.is_structured()) popEmptyElements(parsed);

    // RawConfig.merge_patch(parsed); // Does NOT overwrite
    RawConfig.update(parsed); // Does overwrite
    return 0;
}

int ConfigManager::dump() {
    logIdentity(RawConfig.dump());
    return 0;
}

IConfigManager::TypeDescriptor ConfigManager::getDescriptorFromJson(const nlohmann::json& json) {
    IConfigManager::TypeDescriptor desc;

    switch(json.type()) {
        case nlohmann::json::value_t::boolean:
            desc = IConfigManager::TypeDescriptor(std::type_identity<bool>{});
        break;

        case nlohmann::json::value_t::number_integer:
            desc = IConfigManager::TypeDescriptor(std::type_identity<int64_t>{});
        break;

        case nlohmann::json::value_t::number_unsigned:
            desc = IConfigManager::TypeDescriptor(std::type_identity<uint64_t>{});
        break;

        case nlohmann::json::value_t::number_float:
            desc = IConfigManager::TypeDescriptor(std::type_identity<float>{});
        break;

        case nlohmann::json::value_t::string:
            desc = IConfigManager::TypeDescriptor(std::type_identity<std::string>{});
        break;

        case nlohmann::json::value_t::object:
        case nlohmann::json::value_t::array:
            if(json.empty()) {
                desc = IConfigManager::TypeDescriptor(std::type_identity<void>{});

            } else {
                desc.nested = new IConfigManager::TypeDescriptor(ConfigManager::getDescriptorFromJson(json.front()));
            }
        break;

        default:
            desc = IConfigManager::TypeDescriptor(std::type_identity<void>{});
        break;
    }

    return desc;
}

void* ConfigManager::getPointerToJson(const nlohmann::json& json) {
    void* hold = nullptr;

    switch(json.type()) {
        case nlohmann::json::value_t::boolean:
            hold = new bool(json.get<bool>());
        break;
        
        case nlohmann::json::value_t::number_integer:
            hold = new int(json.get<int64_t>());
        break;

        case nlohmann::json::value_t::number_unsigned:
            hold = new int(json.get<uint64_t>());
        break;

        case nlohmann::json::value_t::number_float:
            hold = new float(json.get<float>());
        break;

        case nlohmann::json::value_t::string:
            hold = new std::string(json.get<std::string>());
        break;

        case nlohmann::json::value_t::object:
        case nlohmann::json::value_t::array:
            std::vector<void*>* array = new std::vector<void*>();

            for(const nlohmann::json& item : json) {
                array->push_back(std::move(getPointerToJson(item)));
            }

            hold = std::move(array);
        break;
    }

    return std::move(hold);
}

std::string ConfigManager::fullkey(const std::vector<std::string>& key) {
    std::string hold;
    for(int i = 0; i < key.size(); i++) {
        hold += key[i];
        if(i + 1 < key.size()) hold += "/";
    }
    return hold;
}

void ConfigManager::popEmptyElements(nlohmann::json& json) {
    if(json.is_array()) {
        for(int i = json.size() - 1; i >= 0; i--) {
            if(json[i].is_structured()) {
                popEmptyElements(json[i]);
            }
            if(json[i].empty()) {
                json.erase(json.begin() + i);
            }
        }
    }
    if(json.is_object()) {
        for(nlohmann::json::iterator it = json.begin(); it != json.end();) {
            if(it->is_structured()) {
                popEmptyElements(*it);
            }
            if(it->empty()) {
                it = json.erase(it);
                continue;
            }
            it++;
        }
    }
}