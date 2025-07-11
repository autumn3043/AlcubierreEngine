#include "module/DataManager/DataManager.h"

#include <nlohmann/json.hpp>

DataManager& DataManager::GetDataManager(const char* userdata, const char* appdata) {
    static DataManager instance(userdata, appdata);
    return instance;
}

#include "module/DataManager/default/enginedata_json.h"
DataManager::DataManager(const char* userdata, const char* appdata) {
    Import(std::string(reinterpret_cast<char*>(formattedjson_enginedata), formattedjson_enginedata_len).c_str());
    if(appdata) Import(appdata);
    if(userdata) Import(userdata);
}

std::any JsonToAny(const nlohmann::json& j) {
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number_integer()) return j.get<int>();
    if (j.is_number_unsigned()) return j.get<unsigned int>();
    if (j.is_number_float()) return j.get<double>();
    if (j.is_string()) return j.get<std::string>();
    if (j.is_array()) {
        std::vector<std::any> arr;
        for (const auto& elem : j) arr.push_back(JsonToAny(elem));
        return arr;
    }
    if (j.is_object()) {
        return j; //Handles json objects, but if the developer still calls them its just gonna break. fuck you.
    }
    return {};
}

void DataManager::Import(const char* in) {
    nlohmann::json rawjson = nlohmann::json::parse(in);

    for(auto& [key, value] : rawjson.items()) {
        if(!blocks.contains(key)) {
            blocks.emplace(key, JsonToAny(value));
        } else {
            std::unordered_map<std::string, std::any>::iterator it = blocks.find(key);

            if(JsonToAny(value).type() == it->second.type()) {
                it->second = JsonToAny(value);
            }
        }
    }
}