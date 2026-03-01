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

                Hash_T loadModel(char* model, uint64_t size) override { return parent->load(model, size); }               
        };

        IModelLoaderImpl IModelLoader_ResourceManager;

    private:
        Hash_T load(char* model, uint64_t size);
        
        std::vector<Hash_T> modelsInMemory;
};

#endif