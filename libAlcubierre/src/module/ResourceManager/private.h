#ifndef ALCENGINE_MODULE_RESOURCEMANAGER
#define ALCENGINE_MODULE_RESOURCEMANAGER

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IGraphicsBackend.h"
//Provides
#include "core/Registry/interface/IModelLoader.h"
#include "core/Registry/interface/IDirector.h"

#include "core/DebugManager/public.h"

class ResourceManagerException : public AlcEngineException {
    public:
        ResourceManagerException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class ResourceManager : public WrapperBaseClass {
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

        int Init();

    public:
        ResourceManager(void* registry);
        ~ResourceManager();

    //Model loading
        public:
            class IModelLoaderImpl : public IModelLoader {
                public:
                    ResourceManager* parent;

                    IModelLoaderImpl(ResourceManager* _parent) : parent(_parent) {}

                    bool isLoaded(uint32_t& modelHash) override { return parent->isLoaded(modelHash); }
                    uint32_t load(std::string& model, bool explicitCreation) override { return parent->load(model, explicitCreation); }
                    int getModelIndex(uint32_t& modelHash) override { return parent->getModelIndex(modelHash); }                    
            };

            IModelLoaderImpl IModelLoader_ResourceManager;

        private:
            bool isLoaded(uint32_t& modelHash);
            uint32_t load(std::string& model, bool explicitCreation);
            int getModelIndex(uint32_t& modelHash);
            const uint32_t hashSeed = 1427359827;
            uint32_t generateHash(const void* data, int dataSize);

            std::unordered_map<uint32_t, int*> modelsInMemory;

    //Scene & rendering management
        public:
            class IDirectorImpl : public IDirector {
                public:
                    ResourceManager* parent;

                    IDirectorImpl(ResourceManager* _parent) : parent(_parent) {}

                    int createScene() override { return parent->createScene(); }
                    int createActor(Vector& worldPosition, std::string& model) override { return parent->createActor(worldPosition, model); }
                    int renderScene() override { return parent->renderScene(); }
            };

            IDirectorImpl IDirector_ResourceManager;

        private:
            int createScene();
            int createActor(Vector& worldPosition, std::string& model);
            int renderScene();

            class Actor {
                public:
                    Actor(ResourceManager* _parent, Vector& _worldPosition, std::string& model);
                    ~Actor() {};

                    Vector worldPosition;

                    uint32_t modelHash;

                private:
                    ResourceManager* parent;
            };

            class Scene {
                public:
                    Scene(ResourceManager* _parent) : parent(_parent) {};
                    ~Scene() {};

                    int createActor(Vector& worldPosition, std::string& model);
                    int render();

                private:
                    ResourceManager* parent;

                    std::vector<Actor> actors;
            };

            std::vector<Scene> scenes;
};

#endif