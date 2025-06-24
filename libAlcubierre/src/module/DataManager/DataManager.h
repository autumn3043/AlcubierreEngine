#ifndef DATAMANAGER_ALE_H
#define DATAMANAGER_ALE_H

#include <string>
#include <list>
#include <nlohmann/json.hpp>
#include <exception>

namespace DataManagerNamespace {
    class version {
        public:
            version() : VersionLiteral{0, 0, 0}, VersionString("0.0.0") {}

            version(int major, int minor, int patch) : 
                VersionLiteral{major, minor, patch} {
                VersionString = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
                
            }

            std::string GetStr() {
                return VersionString;
            }

            int GetIndex(int i) {
                return VersionLiteral[i];
            }

        private:
            std::string VersionString;
            int VersionLiteral[3];
    };

    struct GLFWHint {
        int key;
        int value;
    };

    struct enginedata {
        std::string Name;
        version Version;
    };

    struct appdata {
        std::string Name;
        version Version;
        std::vector<GLFWHint> Hints;
    };

    struct userdata {
        int DisplayWidth;
        int DisplayHeight;
    };
}

class DataManager {
    public:
        static DataManager& GetDataManager();

        const DataManagerNamespace::enginedata& GetEngineData() const {return ENGINEDATA;}
        const DataManagerNamespace::appdata& GetAppData() const {return APPDATA;}
        const DataManagerNamespace::userdata& GetUserData() const {return USERDATA;}

        class AlcFs {
            public:
                static int WriteLn(std::string path, std::string line);
        };

    private:
        DataManager();

        DataManagerNamespace::enginedata ENGINEDATA;
        DataManagerNamespace::appdata APPDATA;
        DataManagerNamespace::userdata USERDATA;
};
#endif