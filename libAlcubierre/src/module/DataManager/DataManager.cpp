#include "module/DataManager/DataManager.h"
using namespace DataManagerNamespace;

#include "module/DebugManager/DebugManager.h"

DataManager& DataManager::GetDataManager() {
    static DataManager instance;

    return instance;
}

DataManager::DataManager() {
    #include "./src/module/DataManager/default/format/userdata_json.h"
    #include "./src/module/DataManager/default/format/appdata_json.h"
    #include "./src/module/DataManager/default/format/enginedata_json.h"

    try {
        nlohmann::json userdataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_userdata), formattedjson_userdata_len));

        USERDATA.DisplayWidth = userdataraw["window_width"];
        USERDATA.DisplayHeight = userdataraw["window_height"];

    } catch (nlohmann::json::exception exception) {
        DebugManager::Log(DebugManager::DebugReport("Failed to parse developer-defined default userdata config file. Referring to internal values."));

        USERDATA.DisplayWidth = 800;
        USERDATA.DisplayHeight = 600;

    } catch (...) {
        throw DebugManager::AlcExcept(DebugManager::DebugReport("Unknown error occurred during assigning userconfig."));
    }

    try {
        nlohmann::json appdataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_appdata), formattedjson_appdata_len));

        APPDATA.Version = version(int(appdataraw["version_major"]), int(appdataraw["version_minor"]), int(appdataraw["version_patch"]));
        APPDATA.Name = appdataraw["name"];
        APPDATA.Hints; //TODO

    } catch (nlohmann::json::exception exception) {
        DebugManager::Log(DebugManager::DebugReport("Failed to parse developer-defined default appdata config file. Referring to internal values."));

        APPDATA.Version = version(0, 0, 0);
        APPDATA.Name = "FALLBACK_WINDOW_TITLE";

    } catch (...) {
        throw DebugManager::AlcExcept(DebugManager::DebugReport("Unknown error occurred during acquiring appdata."));
    }

    try {
        nlohmann::json enginedataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_enginedata), formattedjson_enginedata_len));
        
        ENGINEDATA.Version = version(int(enginedataraw["version_major"]), int(enginedataraw["version_minor"]), int(enginedataraw["version_patch"]));
        ENGINEDATA.Name = enginedataraw["name"];
    
    } catch (nlohmann::json::exception exception) {
        DebugManager::Log(DebugManager::DebugReport("Failed to parse Alcubierre internal data."));
    }
}