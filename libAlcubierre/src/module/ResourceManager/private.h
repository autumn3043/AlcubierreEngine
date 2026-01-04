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
                    uint32_t load(std::vector<Vector>& modelData, std::vector<uint32_t>& modelIndices, bool explicitCreation) override { return parent->load(modelData, modelIndices, explicitCreation); }                    
            };

            IModelLoaderImpl IModelLoader_ResourceManager;

        private:
            bool isLoaded(uint32_t& modelHash);
            uint32_t load(std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices, bool explicitCreation);
            const uint32_t hashSeed = 1427359827;
            uint32_t generateHash(void* data, uint32_t dataSize);

            struct Model {
                uint32_t hash;
            };

            std::vector<Model> loadedModels;

    //Scene & rendering management
        public:
            class IDirectorImpl : public IDirector {
                public:
                    ResourceManager* parent;

                    IDirectorImpl(ResourceManager* _parent) : parent(_parent) {}

                    int createScene() override { return parent->createScene(); }
                    int createActor(Vector& worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices) override { return parent->createActor(worldPosition, modelVertices, modelIndices); }
                    int renderScene() override { return parent->renderScene(); }
            };

            IDirectorImpl IDirector_ResourceManager;

        private:
            int createScene();
            int createActor(Vector& worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices);
            int renderScene();

            class Actor {
                public:
                    Actor(ResourceManager* _parent, Vector& _worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices);
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

                    int createActor(Vector& worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices);
                    int render();

                private:
                    ResourceManager* parent;

                    std::vector<Actor> actors;
            };

            std::vector<Scene> scenes;
            std::vector<uint32_t> modelHashes;
};

#endif