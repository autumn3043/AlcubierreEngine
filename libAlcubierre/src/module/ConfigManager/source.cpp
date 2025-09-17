#include "module/ConfigManager/public.h"
#include "module/ConfigManager/private.h"

#include <cstring>

ModuleRegistryBundle ConfigManagerWrapper::bundle(
    []() -> WrapperBaseClass* { return new ConfigManagerWrapper(); },
    "MODULE_CONFIGMANAGER"
);

ConfigManager::ConfigManager() : IConfigManager_ConfigManager(this) {}

ConfigManager::~ConfigManager() {
    if(PrivatePtr) delete PrivatePtr;
}

int ConfigManager::WakeImpl() {
    if(!PrivatePtr) {
        PrivatePtr = new ConfigManagerImpl();
        return 1;
    } else {
        return 0;
    }
}

ConfigManagerImpl::ConfigManagerImpl() {
    RawConfig = nlohmann::json();
}

ConfigManagerImpl::~ConfigManagerImpl() {
    // DM().Log(RawConfig.dump());
}

int ConfigManager::Get_Impl(IConfigManager::Container& v_out) {
    return PrivatePtr->get_impl(v_out);
}

int ConfigManager::Set_Impl(IConfigManager::Container& v_in) {
    return PrivatePtr->set_impl(v_in);    
}

int ConfigManagerImpl::get_impl(IConfigManager::Container& v_out) {
    //Does a value exist at this key?
    nlohmann::json* json = &RawConfig;
    for(int i = 0; i < v_out.key.size(); i++) {
        if(!json->contains(v_out.key[i])) {
            loc_Log("Failed to get value at key '" + fullkey(v_out.key) +"' because it did not exist");
            return 1;
        } else json = &((*json)[v_out.key[i]]);
    }

    //Does its type match the expected return type?
    if((*v_out.t_info != GetDescriptorFromJson(*json))) {
        loc_Log("Stored value at key '" + fullkey(v_out.key) + "' was: " + GetDescriptorFromJson(*json).Type() + " which did not match the requested type: " + v_out.t_info->Type(), 1);
        return 1;
    }

    //If yes to both, get, return and move a pointer to it.
    v_out.ptr = std::move(GetPointerToJson(*json));
    return 0;
}

IConfigManager::TypeDescriptor ConfigManagerImpl::GetDescriptorFromJson(const nlohmann::json& json) {
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
            desc = IConfigManager::TypeDescriptor(std::type_identity<double>{});
        break;

        case nlohmann::json::value_t::string:
            desc = IConfigManager::TypeDescriptor(std::type_identity<std::string>{});
        break;

        case nlohmann::json::value_t::object:
        case nlohmann::json::value_t::array:
            if(json.empty()) {
                desc = IConfigManager::TypeDescriptor(std::type_identity<void>{});

            } else {
                desc.nested = new IConfigManager::TypeDescriptor(ConfigManagerImpl::GetDescriptorFromJson(json.front()));
            }
        break;

        default:
            desc = IConfigManager::TypeDescriptor(std::type_identity<void>{});
        break;
    }

    return desc;
}

void* ConfigManagerImpl::GetPointerToJson(const nlohmann::json& json) {
    void* hold = nullptr;

    switch(json.type()) {
        case nlohmann::json::value_t::boolean:
            hold = new bool(json.get<bool>());
        break;
        
        case nlohmann::json::value_t::number_integer:
            hold = new int64_t(json.get<int64_t>());
        break;

        case nlohmann::json::value_t::number_unsigned:
            hold = new uint64_t(json.get<uint64_t>());
        break;

        case nlohmann::json::value_t::number_float:
            hold = new double(json.get<double>());
        break;

        case nlohmann::json::value_t::string:
            hold = new std::string(json.get<std::string>());
        break;

        case nlohmann::json::value_t::object:
        case nlohmann::json::value_t::array:
            std::vector<void*>* array = new std::vector<void*>();

            for(const nlohmann::json& item : json) {
                array->push_back(std::move(GetPointerToJson(item)));
            }

            hold = std::move(array);
        break;
    }

    return hold;
}

int ConfigManagerImpl::set_impl(IConfigManager::Container& v_in) {
    // DM().Log("Inserting " + fullkey(v_in.key) + "\nState prior: " + RawConfig.dump());
    std::string input = *static_cast<std::string*>(v_in.ptr);
    nlohmann::json parsed;

    try {
        parsed = nlohmann::json::parse(input);
    } catch (const nlohmann::json::parse_error& E) {
        loc_Log("Failed to parse string: '" + input + "'\nVerbose error: " + E.what(), 5);
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
                
                if(*v_in.t_info == GetDescriptorFromJson((*json)[notch])) {
                    (*json)[notch].swap(parsed);
                    return 0;
                } 

                loc_Log("Failed to insert key '" + fullkey(v_in.key) + "' of type: " + v_in.t_info->Type() + " due to mismatch with stored type: " + GetDescriptorFromJson((*json)[notch]).Type(), 1);
                return 1;
            }
        }
    } catch (const std::exception& E) {
        loc_Log("Failed to insert key '" + fullkey(v_in.key) + "' of type: " + v_in.t_info->Type() + "\nVerbose error: " + E.what() + "\n" + RawConfig.dump(), 5);
        return 1;
    }

    return -1;
}

std::string ConfigManagerImpl::fullkey(const std::vector<std::string>& key) {
    std::string hold;
    for(int i = 0; i < key.size(); i++) {
        hold += key[i];
        if(i + 1 < key.size()) hold += "/";
    }
    return hold;
}