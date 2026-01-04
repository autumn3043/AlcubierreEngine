#include "module/ResourceManager/private.h"

// #include <tiny_obj_loader.h>
#include <xxhash.h>

inline void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "ResourceManager"), Write); }

ModuleRegistryBundle ResourceManager::bundle(
    [](void* registry) -> WrapperBaseClass* { return new ResourceManager(registry); },
    {MODEL_LOADER, DIRECTOR},
    {GRAPHICS_BACKEND},
    "ResourceManager"
);

ResourceManager::ResourceManager(void* registry) 
    :   IModelLoader_ResourceManager(this),
        IDirector_ResourceManager(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{MODEL_LOADER, &IModelLoader_ResourceManager}, {DIRECTOR, &IDirector_ResourceManager}};
        Init();
    }

ResourceManager::~ResourceManager() {
    
}

int ResourceManager::Init() {
    return 0;
}

//Model loading
    bool ResourceManager::isLoaded(uint32_t& modelHash) {
        for(int i = 0; i < loadedModels.size(); i++) {
            if(loadedModels[i].hash == modelHash) {
                logIdentity("Detected reuse of existing loaded model");
                return true;
            }
        }

        return false;
    }

    uint32_t ResourceManager::load(std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices, bool explicitCreation) {
        uint32_t modelHash = generateHash(modelVertices.data(), sizeof(Vector) * modelVertices.size());

        //TODO: Gracefully handle explicitly created duplicates (which need their own unique hash)
            explicitCreation = false;

        if(explicitCreation || !isLoaded(modelHash)) {
            //tinyobj time
            //Load needs to use raw .obj data when tinyobjloader is implemented

            IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));

            GB->createObjectBuffer(modelHash, modelVertices, modelIndices);

            loadedModels.push_back({modelHash});

            logIdentity("Loaded model with hash [" + std::to_string(modelHash) + "] into a buffer");
        }

        return modelHash;
    }

    uint32_t ResourceManager::generateHash(void* data, uint32_t dataSize) {
        return XXH32(data, dataSize, hashSeed);
    }    

//Scene & rendering management
    int ResourceManager::createScene() {
        int sceneIndex = scenes.size();
        scenes.emplace_back(this);

        return sceneIndex;
    }

    int ResourceManager::createActor(Vector& worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices) {
        Scene& scene = scenes[0];

        return scene.createActor(worldPosition, modelVertices, modelIndices);
    }

    int ResourceManager::renderScene() {
        Scene& scene = scenes[0];

        return scene.render();
    }

    ResourceManager::Actor::Actor(ResourceManager* _parent, Vector& _worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices) : parent(_parent), worldPosition(_worldPosition) {
        IModelLoader* ML = dynamic_cast<IModelLoader*>(parent->registry_ptr->FetchService(MODEL_LOADER));

        modelHash = ML->load(modelVertices, modelIndices); //If model is already loaded, does nothing, otherwise, generate a memory buffer for it

        logIdentity("Created a new actor at " + std::to_string(_worldPosition.x) + "x, " + std::to_string(_worldPosition.y) + "y, " + std::to_string(_worldPosition.x) + "z");
    }

    int ResourceManager::Scene::createActor(Vector& worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices) {
        int actorIndex = actors.size();
        actors.emplace_back(parent, worldPosition, modelVertices, modelIndices);

        return actorIndex;
    }

    int ResourceManager::Scene::render() {
        IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(parent->registry_ptr->FetchService(GRAPHICS_BACKEND));

        GB->clearFrame();

        for(int i = 0; i < actors.size(); i++) {
            GB->addObjectToFrame(actors[i].modelHash, actors[i].worldPosition);
        }

        return GB->drawFrame();
    }