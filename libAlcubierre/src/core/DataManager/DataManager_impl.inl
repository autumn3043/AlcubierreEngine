#ifndef DATAMANAGER_IMPLEMENTATION_ALC_H
#define DATAMANAGER_IMPLEMENTATION_ALC_H

#include <vector>
#include <type_traits>
#include <nlohmann/json.hpp>

#include "core/DebugManager/DebugManager.h"

class DataManager::JsonHolder {
    public:
        nlohmann::json json;
};

//struct to handle extracting the subtype of a std::vector, not included in header cuz json required
template <typename T>
struct is_std_vector : std::false_type {};

template <typename U, typename Alloc>
struct is_std_vector<std::vector<U, Alloc>> : std::true_type {
    using value_type = U;
};

template <typename T>
T DataManager::Get(std::string key, T fallback) {
    T hold;
    try {
        hold = GetFromJson<T>(*pjsonholder, key);
    } catch (AlcExceptions::ConfigParseException E) {
        hold = fallback;
        DebugManager::Log(E);
    } catch (...) {
        throw;
    }
    return hold;
}

template <typename T>
T DataManager::Get(std::string key) {
    return GetFromJson<T>(*pjsonholder, key);
}

template <typename T>
T DataManager::GetFromJson(const JsonHolder& jsonholder, const std::string& key, bool R, int I) {
    std::string keyname = key;
    if(!jsonholder.json.contains(key) && !R) {
        throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON was missing requested key: " + keyname));
    }

    nlohmann::json data;
    if(R) { //array
        data = jsonholder.json;
        keyname = "<array access> loop: " + std::to_string(I);
    } else { 
        data = jsonholder.json[key];
    }

    nlohmann::json::value_t dataT = data.type();
    
    if (dataT == nlohmann::json::value_t::boolean) {
        if constexpr(std::is_same_v<T, bool>) {
            return data.get<T>();
        } else {
            throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name()));
        }
    } else if (dataT == nlohmann::json::value_t::number_integer) {
        if constexpr(std::is_same_v<T, int>) {
            return data.get<T>();
        } else {
            throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name() + " (int)"));
        }
    } else if (dataT == nlohmann::json::value_t::number_unsigned) {
        if constexpr(std::is_same_v<T, int>) {
            return data.get<T>();
        } else {
            throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name() + " (uint)"));
        }
    } else if (dataT == nlohmann::json::value_t::number_float) {
        if constexpr(std::is_same_v<T, int>) {
            return data.get<T>();
        } else {
            throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name() + " double"));
        }
    } else if (dataT == nlohmann::json::value_t::string) {
        if constexpr(std::is_same_v<T, std::string>) {
            return data.get<T>();
        } else {
           throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name()));
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
           throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name()));
        }
    } else {
        throw AlcExceptions::AlcExcept(AlcExceptions::ConfigParseException("Config JSON type mismatch for: "  + keyname + " of type: " + data.type_name()));
    }

    T debugshutup;
    return debugshutup;
}

#endif