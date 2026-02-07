#ifndef ALCENGINE_MODULE_RESOURCEMANAGER_H
#define ALCENGINE_MODULE_RESOURCEMANAGER_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IGraphicsBackend.h"
//Provides
#include "core/Registry/interface/IModelLoader.h"

#include "core/DebugManager/public.h"

class ResourceManagerException : public AlcEngineException {
    public:
        ResourceManagerException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class ResourceManager : public WrapperBaseClass {
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

    public:
        ResourceManager(void* registry);
        ~ResourceManager();

        void init();

        class IModelLoaderImpl : public IModelLoader {
            public:
                ResourceManager* parent;

                IModelLoaderImpl(ResourceManager* _parent) : parent(_parent) {}                
        };

        IModelLoaderImpl IModelLoader_ResourceManager;

    private:
        bool isLoaded(uint32_t& modelHash);
        Mesh3D* load(uint32_t& modelHash, rawModelData& modelData);
        Mesh3D* getModel(uint32_t modelHash);

        std::unordered_map<uint32_t, Mesh3D> modelsInMemory;
};

#endif