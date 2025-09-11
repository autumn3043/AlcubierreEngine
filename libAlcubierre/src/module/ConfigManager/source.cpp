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
    if(!RawConfig.contains(v_out.key)) {
        return 1;
    }

    //Does its type match the expected return type?
    if(v_out.t_info->Type() != GetDescriptorFromJson(RawConfig[v_out.key]).Type()) {
        return 1;
    }

    //If yes to both, get, return and move a pointer to it.
    v_out.ptr = std::move(GetPointerToJson(RawConfig[v_out.key]));
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
    std::string input = *static_cast<std::string*>(v_in.ptr);
    nlohmann::json parsed;

    try {
        parsed = nlohmann::json::parse(input);
    } catch (nlohmann::json::parse_error E) {
        DM().Log("Failed to parse string:\n" + input + "\nJson error message:" + E.what());
        return 1;
    } 

    if(RawConfig.contains(v_in.key)) {
        if(v_in.t_info->Type() == GetDescriptorFromJson(RawConfig[v_in.key]).Type()) {
            RawConfig[v_in.key].swap(parsed);
            return 0;
        } else {
            return 1;
        }

    } else {
        RawConfig.push_back(nlohmann::json::object_t::value_type(v_in.key, parsed));
        return 0;
    }
}