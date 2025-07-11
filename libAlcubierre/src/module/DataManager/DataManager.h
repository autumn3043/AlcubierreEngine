#ifndef DATAMANAGER_ALC_H
#define DATAMANAGER_ALC_H

#include "module/DebugManager/DebugManager.h"

#include <string>
#include <unordered_map>
#include <any>
#include <vector>

class DataManager {
    public:
        static DataManager& GetDataManager(const char* userdata = nullptr, const char* appdata = nullptr);

        template <typename T>
        T Get(const std::string& key, T fallback) {
            try {
                return Get<T>(key);
            } catch (AlcExceptions::ConfigValMissing E) {
                return fallback;
            } catch (...) {
                throw;
            }
        }

        template <typename T>
        T Get(const std::string& key) {
            if(blocks.contains(key)) {
                T hold;
                try {
                    hold = std::any_cast<T>(blocks[key]);
                    return hold;
                } catch (std::bad_any_cast E) {
                    throw AlcExceptions::AlcExcept(AlcExceptions::DebugReport("Bad type: " + std::string(typeid(T).name())));
                }
            } else {
                throw AlcExceptions::AlcExcept(AlcExceptions::ConfigValMissing());
            }
        }

    private:
        DataManager(const char* userdata, const char* appdata);

        std::unordered_map<std::string, std::any> blocks;

        void Import(const char* in);

        //Each get call provides its own fallback, but values in User will overwrite those in App, and App will overwrite Lib, providing support for default values via downstream flow.
};

#endif