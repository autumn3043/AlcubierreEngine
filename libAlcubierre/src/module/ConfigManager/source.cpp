#include "module/ConfigManager/public.h"
#include "module/ConfigManager/private.h"

#include <cstring>

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

int ConfigManager::Get_Impl(IConfigManager::Container& v_out) {
    //Does a value exist at this key?
    if(!PrivatePtr->RawConfig.contains(v_out.key)) {
        return 1;
    }

    //Does its type match the expected return type?
    if(v_out.t_info->Type() != PrivatePtr->GetDescriptorFromJson(PrivatePtr->RawConfig[v_out.key]).Type()) {
        return 1;
    }

    //If yes to both, get, return and move a pointer to it.
    v_out.ptr = std::move(PrivatePtr->GetPointerToJson(PrivatePtr->RawConfig[v_out.key]));
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

        case nlohmann::json::value_t::array:
            if(json.empty()) {
                desc = IConfigManager::TypeDescriptor(std::type_identity<void>{});

            } else {
                desc.nested = new IConfigManager::TypeDescriptor(ConfigManagerImpl::GetDescriptorFromJson(json.front()));
            }

        break;

        //Because we're using templates, we can't handle for nlohmann::json native or binary_t types. If the user chooses to use them anyway, they are passed without type checking and we trust that the user isn't dumb.
        default:
            desc.type = "unknown";
        break;
    }

    return std::move(desc);
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

int ConfigManager::Set_Impl(IConfigManager::Container& v_in) {
    std::string input = *static_cast<std::string*>(v_in.ptr);
    nlohmann::json json;

    try {
        json = nlohmann::json::parse(input.c_str());
    } catch (nlohmann::json::parse_error E) {
        DM().Log("Failed to parse string:\n" + input + "\nJson error message:" + E.what());
        return 1;
    } catch (...) {
        throw;
    }

    PrivatePtr->SetFromParsed(json);
    return 0;
}

ModuleRegistryBundle ConfigManagerWrapper::bundle(
    []() -> WrapperBaseClass* { return new ConfigManagerWrapper(); },
    "MODULE_CONFIGMANAGER"
);

void ConfigManagerImpl::SetFromParsed(const nlohmann::json& json) {
    for(auto& [key, val] : json.items()) {
        if(RawConfig.contains(key)) {
            if(RawConfig[key].type() == val.type()) {
                RawConfig[key] = val;
            }
        } else {
            RawConfig[key] = val;
        }
    }

    DM().Log(RawConfig.dump());
}

ConfigManagerImpl::ConfigManagerImpl() {
    RawConfig = nlohmann::json();
}