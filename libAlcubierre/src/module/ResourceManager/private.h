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

        int init();

        class IModelLoaderImpl : public IModelLoader {
            public:
                ResourceManager* parent;

                IModelLoaderImpl(ResourceManager* _parent) : parent(_parent) {} 

                int loadModel(rawModelData model) override { return parent->load(model); }               
        };

        IModelLoaderImpl IModelLoader_ResourceManager;

    private:
        int load(IModelLoader::rawModelData& modelData);
        
        std::vector<uint32_t> modelsInMemory;
};

#endif