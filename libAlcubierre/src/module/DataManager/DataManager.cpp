#include "module/DataManager/DataManager.h"
using namespace DataManagerNamespace;

#include "module/DebugManager/DebugManager.h"

DataManager& DataManager::GetDataManager() {
    static DataManager instance;

    return instance;
}

DataManager::DataManager() {
    #include "module/DataManager/default/format/userdata_json.h"
    #include "module/DataManager/default/format/appdata_json.h"
    #include "module/DataManager/default/format/enginedata_json.h"

    try {
        nlohmann::json userdataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_userdata), formattedjson_userdata_len));

        USERDATA.DisplayWidth = userdataraw["window_width"].get<int>();
        USERDATA.DisplayHeight = userdataraw["window_height"].get<int>();

    } catch (nlohmann::json::exception exception) {
        DebugManager::Log("Failed to parse developer-defined default userdata config file. Referring to internal values.");

        USERDATA.DisplayWidth = 800;
        USERDATA.DisplayHeight = 600;

    } catch (...) {
        throw AlcExceptions::ConfigAssignFail();
    }

    try {
        nlohmann::json appdataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_appdata), formattedjson_appdata_len));

        APPDATA.Version = version(int(appdataraw["version_major"].get<int>()), int(appdataraw["version_minor"].get<int>()), int(appdataraw["version_patch"].get<int>()));
        APPDATA.Name = appdataraw["name"].get<std::string>();
        APPDATA.Hints; //TODO

    } catch (nlohmann::json::exception exception) {
        DebugManager::Log("Failed to parse developer-defined default userdata config file. Referring to internal values.");

        APPDATA.Version = version(0, 0, 0);
        APPDATA.Name = "FALLBACK_WINDOW_TITLE";

    } catch (...) {
        throw AlcExceptions::ConfigAssignFail();;
    }

    try {
        nlohmann::json enginedataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_enginedata), formattedjson_enginedata_len));
        
        ENGINEDATA.Version = version(int(enginedataraw["version_major"].get<int>()), int(enginedataraw["version_minor"].get<int>()), int(enginedataraw["version_patch"].get<int>()));
        ENGINEDATA.Name = enginedataraw["name"].get<std::string>();
    
    } catch (nlohmann::json::exception exception) {
        throw AlcExceptions::ConfigAssignFail();
    }
}

#include <fstream>
int DataManager::AlcFs::WriteLn(std::string path, std::string line) {
    try {
        std::ofstream output;
        output.open(path, std::ios::app);
        output << line << std::endl;
        output.close();
        return 0;
    } catch(...) {
        throw AlcExceptions::IOError();
    }
}