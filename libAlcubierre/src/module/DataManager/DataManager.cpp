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

    nlohmann::json userdataraw;
    // try {
        //read from user config file
        // USERDATA = UserDataFromJson(userdataraw);
    // } catch (const std::exception _E1) {
        // DebugManager::Log(AlcExceptions::DebugReport(_E1.what()));
        DebugManager::Log("Falling back to internal default userconfig");

        userdataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_userdata), formattedjson_userdata_len));
        USERDATA = UserDataFromJson(userdataraw);
    // }

    nlohmann::json appdataraw;
    // try {
        //read from app config h file
        // appdata = appdataFromJson(appdataraw);
    // } catch (const std::exception _E1) {
        // DebugManager::Log(AlcExceptions::DebugReport(_E1.what()));
        DebugManager::Log("Falling back to internal default app config");

        appdataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_appdata), formattedjson_appdata_len));
        APPDATA = AppDataFromJson(appdataraw);
    // }

    nlohmann::json enginedataraw;
    try {
        enginedataraw = nlohmann::json::parse(std::string(reinterpret_cast<char*>(formattedjson_enginedata), formattedjson_enginedata_len));
        ENGINEDATA = EngineDataFromJson(enginedataraw);
    } catch (const std::exception _E) {
        DebugManager::Log(AlcExceptions::DebugReport(_E.what()));
    }

}

DataManagerNamespace::userdata DataManager::UserDataFromJson(nlohmann::json json) {
    DataManagerNamespace::userdata hold;

    hold.WindowWidth = json["window_width"].get<int>();
    hold.WindowHeight = json["window_height"].get<int>();

    return hold;
}

DataManagerNamespace::appdata DataManager::AppDataFromJson(nlohmann::json json) {
    DataManagerNamespace::appdata hold;

    hold.Version = version(int(json["version_major"].get<int>()), int(json["version_minor"].get<int>()), int(json["version_patch"].get<int>()));
    hold.Name = json["name"].get<std::string>();

    return hold;
}

DataManagerNamespace::enginedata DataManager::EngineDataFromJson(nlohmann::json json) {
    DataManagerNamespace::enginedata hold;

    hold.Version = version(int(json["version_major"].get<int>()), int(json["version_minor"].get<int>()), int(json["version_patch"].get<int>()));
    hold.Name = json["name"].get<std::string>();

    return hold;
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