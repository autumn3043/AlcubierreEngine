#ifndef ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H
#define ALCENGINE_MODULE_CONFIGMANAGER_PUBLIC_H

#include "core/Registry/public.h"

//Services
#include "core/Registry/interface/IConfigManager.h"

#include "core/DebugManager/public.h"

#include <unordered_map>
#include <xxhash.h>

//OPT: This is called on every key in memory every time any key is added or adjusted. Rehashing is bad, cache.
namespace std {
    template <>
    struct hash<IConfigManager::key_t> {
        uint32_t operator()(const IConfigManager::key_t& key) const noexcept {
            std::string keyString = key.toString();
            return XXH32(keyString.c_str(), sizeof(const char) * keyString.length(), 1);
        }
    };
}

class ConfigManagerException : public AlcEngineException {
    public:
        ConfigManagerException(std::string message, int priority = 1) : AlcEngineException(DebugReport(message, priority, "ConfigManager")) {}
};

#include <nlohmann/json.hpp>

class ConfigManager : public WrapperBaseClass{
    public:
        int get(IConfigManager::Container& v_out);
        int set(IConfigManager::Container& v_in);
        int setParse(IConfigManager::Container& v_in);
        int dump();
        int registerCallback(IConfigManager::key_t& key, std::function<void()> fn);

        class IConfigManagerImpl : public IConfigManager {
            public:
                ConfigManager* Parent;

                IConfigManagerImpl(ConfigManager* _parent) : Parent(_parent) {}

                int getInternal(IConfigManager::Container& v_out) override { return Parent->get(v_out); }
                int setInternal(IConfigManager::Container& v_in) override { return Parent->set(v_in); }
                int setParseInternal(IConfigManager::Container& v_in) override { return Parent->setParse(v_in); }
                int dump() override { return Parent->dump(); }
                int registerCallback(IConfigManager::key_t key, std::function<void()> fn) override { return Parent->registerCallback(key, fn); }
        };

        ConfigManager(void* registry);
        ~ConfigManager();

        IConfigManagerImpl IConfigManager_ConfigManager;
        
    private:
        Registry* registry_ptr = nullptr;
        static ModuleRegistryBundle bundle;

        int init();

        nlohmann::json RawConfig;

        IConfigManager::TypeDescriptor getDescriptorFromJson(const nlohmann::json& json);
        void* getPointerToJson(const nlohmann::json& json);
        void popEmptyElements(nlohmann::json& json);
        std::unordered_map<IConfigManager::key_t, std::vector<std::function<void()>>> callbacks;
};

#endif