#include "module/ConfigManager/private.h"
#include "module/ConfigManager/public.h"

ConfigManager::ConfigManager(const char* userdata, const char* appdata) {
    PrivatePtr = new ConfigManagerIMPL();
    PrivatePtr->Import(userdata, appdata);
}

#include "module/ConfigManager/default/enginedata_json.h"
int ConfigManagerIMPL::Import(const char* userdata, const char* appdata) {
    std::vector<const char*> rawfiles;
    rawfiles.push_back(reinterpret_cast<const char*>(formattedjson_enginedata));
    rawfiles.push_back(appdata);
    rawfiles.push_back(userdata);

    int successes = 0;
    for(const char* rawjson : rawfiles) {
        if(!rawjson) break;
        nlohmann::json parse = nlohmann::json::parse(rawjson);
        for(auto& [key, val] : parse.items()) {
            if(!json.contains(key)) {
                json[key] = val;
            } else {
                if(json[key].type() == val.type()) {
                    json[key] = val;
                }
            }
        }
        successes++;
    }

    return successes;
}

//struct to handle extracting the subtype of a std::vector
template <typename T>
struct is_std_vector : std::false_type {};

template <typename U, typename Alloc>
struct is_std_vector<std::vector<U, Alloc>> : std::true_type {
    using value_type = U;
};

template <typename T>
T ConfigManagerIMPL::GetFromJson(const nlohmann::json& jsonholder, const std::string& key, bool R, int I) {
    std::string keyname = key;
    if(!jsonholder.contains(key) && !R) {
        throw ConfigManagerException("Config JSON was missing requested key: " + keyname);
    }

    nlohmann::json data;
    if(R) { //array
        data = jsonholder;
        keyname = "<array access> loop: " + std::to_string(I);
    } else { 
        data = jsonholder[key];
    }

    nlohmann::json::value_t dataT = data.type();
    
    if (dataT == nlohmann::json::value_t::boolean) {
        if constexpr(std::is_same_v<T, bool>) {
            return data.get<T>();
        } else {
            throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name());
        }
    } else if (dataT == nlohmann::json::value_t::number_integer) {
        if constexpr(std::is_same_v<T, int>) {
            return data.get<T>();
        } else {
            throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name() + " (int)");
        }
    } else if (dataT == nlohmann::json::value_t::number_unsigned) {
        if constexpr(std::is_same_v<T, int>) {
            return data.get<T>();
        } else {
            throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name() + " (uint)");
        }
    } else if (dataT == nlohmann::json::value_t::number_float) {
        if constexpr(std::is_same_v<T, int>) {
            return data.get<T>();
        } else {
            throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name() + " double");
        }
    } else if (dataT == nlohmann::json::value_t::string) {
        if constexpr(std::is_same_v<T, std::string>) {
            return data.get<T>();
        } else {
        throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name());
        }
    } else if (dataT == nlohmann::json::value_t::array) {
        if constexpr(is_std_vector<T>::value) {
            using elementType = typename is_std_vector<T>::value_type;
            std::vector<elementType> hold;
            int i = 0;
            for(const nlohmann::json& item : data.get<std::vector<nlohmann::json>>()) {
                JsonHolder _hold;
                _hold.json = item;
                hold.push_back(GetFromJson<elementType>(_hold, "", true, i));
                ++i;
            }
            return(hold);
        } else {
        throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name());
        }
    } else {
        throw ConfigManagerException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name());
    }

    T debugshutup;
    return debugshutup;
}

template <typename T>
T ConfigManager::Get(std::string key, T fallback) {
    T hold;
    try {
        hold = PrivatePtr->GetFromJson<T>(PrivatePtr->json, key);
    } catch (ConfigManagerException E) {
        hold = fallback;
        DM.Log(E);
    } catch (...) {
        throw;
    }
    return hold;
}

template <typename T>
T ConfigManager::Get(std::string key) {
    return PrivatePtr->GetFromJson<T>(PrivatePtr->json, key);
}