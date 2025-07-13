#include "core/DataManager/DataManager.h"

#include <nlohmann/json.hpp>

DataManager::DataManager(const char* userdata, const char* appdata) {
    pjsonholder = std::make_unique<JsonHolder>();
    Import(userdata, appdata);
}

DataManager::~DataManager() = default;

#include "core/DataManager/default/enginedata_json.h"
int DataManager::Import(const char* userdata, const char* appdata) {
    std::vector<const char*> rawfiles;
    rawfiles.push_back(reinterpret_cast<const char*>(formattedjson_enginedata));
    rawfiles.push_back(appdata);
    rawfiles.push_back(userdata);

    int successes = 0;
    for(const char* rawjson : rawfiles) {
        if(!rawjson) break;
        nlohmann::json parse = nlohmann::json::parse(rawjson);
        for(auto& [key, val] : parse.items()) {
            if(!pjsonholder->json.contains(key)) {
                pjsonholder->json[key] = val;
            } else {
                if(pjsonholder->json[key].type() == val.type()) {
                    pjsonholder->json[key] = val;
                }
            }
        }
        successes++;
    }

    return successes;
}

