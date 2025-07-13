#ifndef DATAMANAGER_ALC_H
#define DATAMANAGER_ALC_H

#include <string>
#include <utility>
#include <memory>

class DataManager {
    public:
        static DataManager& GetDataManager(const char* userdata = nullptr, const char* appdata = nullptr) {
            static DataManager Instance(userdata, appdata);
            return Instance;
        }

        template <typename T>
        T Get(std::string key, T fallback);

        template <typename T>
        T Get(std::string key);

    private:
        class JsonHolder;
        std::unique_ptr<JsonHolder> pjsonholder;

        DataManager(const char* userdata = nullptr, const char* appdata = nullptr);
        ~DataManager();

        int Import(const char* userdata, const char* appdata);

        template <typename T>
        T GetFromJson(const JsonHolder& jsonholder, const std::string& key, bool R = false, int I = 0);
};

#include "core/DataManager/DataManager_impl.inl"

#endif