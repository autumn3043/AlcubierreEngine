#include "module/ResourceManager/private.h"

#include <tiny_obj_loader.h>
#include <xxhash.h>

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "ResourceManager"), Write); }

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
        return modelsInMemory.contains(modelHash);
    }

    uint32_t ResourceManager::load(std::string& model, bool explicitCreation) {

        uint32_t modelHash = generateHash(model.c_str(), model.size());

        if(!isLoaded(modelHash)) {
            std::vector<Vector> modelVertices;
            std::vector<uint32_t> modelIndices;

            tinyobj::ObjReader reader;
            tinyobj::ObjReaderConfig readerConfig;
            reader.ParseFromString(model, std::string(), readerConfig);

            tinyobj::attrib_t attributes = reader.GetAttrib();
            std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
            std::vector<tinyobj::material_t> materials = reader.GetMaterials();

            if(!reader.Warning().empty()) logIdentity("[tinyobj]: " + reader.Warning(), 1);
            if(!reader.Error().empty()) logIdentity("[tinyobj]: " + reader.Error(), 2);

            for(int i = 0; i < shapes.size(); i++) {
                tinyobj::shape_t& shape = shapes[i];

                for(int j = 0; j < shape.mesh.indices.size(); j++) {
                    tinyobj::index_t& index = shape.mesh.indices[j];

                    float x = attributes.vertices[3 * index.vertex_index + 0];
                    float y = attributes.vertices[3 * index.vertex_index + 1];
                    float z = attributes.vertices[3 * index.vertex_index + 2];

                    modelVertices.push_back({x, y, z});
                }
            }

            //temp
                for(int i = 0; i < modelVertices.size(); i++) {
                    modelIndices.push_back(i);
                }

            IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));

            IGraphicsBackend::modelData modelData = {
                .vertices = modelVertices,
                .indices = modelIndices
            };

            modelsInMemory.emplace(modelHash, nullptr);
            GB->createObjectBuffer(modelsInMemory[modelHash], modelData);
        }

        return modelHash;
    }

    int ResourceManager::getModelIndex(uint32_t& modelHash) {
        return *modelsInMemory[modelHash];
    }

    uint32_t ResourceManager::generateHash(const void* data, int dataSize) {
        return XXH32(data, dataSize, hashSeed);
    }    

//Scene & rendering management
    int ResourceManager::createScene() {
        int sceneIndex = scenes.size();
        scenes.emplace_back(this);

        return sceneIndex;
    }

    int ResourceManager::createActor(Vector& worldPosition, std::string& model) {
        Scene& scene = scenes[0];

        return scene.createActor(worldPosition, model);
    }

    int ResourceManager::renderScene() {
        Scene& scene = scenes[0];

        return scene.render();
    }

    ResourceManager::Actor::Actor(ResourceManager* _parent, Vector& _worldPosition, std::string& model) : parent(_parent), worldPosition(_worldPosition) {
        IModelLoader* ML = dynamic_cast<IModelLoader*>(parent->registry_ptr->FetchService(MODEL_LOADER));

        modelHash = ML->load(model); //If model is already loaded, does nothing, otherwise, generate a memory buffer for it

        logIdentity("Created a new actor at " + std::to_string(_worldPosition.x) + "x, " + std::to_string(_worldPosition.y) + "y, " + std::to_string(_worldPosition.x) + "z");
    }

    int ResourceManager::Scene::createActor(Vector& worldPosition, std::string& model) {
        int actorIndex = actors.size();
        actors.emplace_back(parent, worldPosition, model);

        return actorIndex;
    }

    int ResourceManager::Scene::render() {
        if(actors.size() == 0) return 1;
        
        IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(parent->registry_ptr->FetchService(GRAPHICS_BACKEND));
        IModelLoader* ML = dynamic_cast<IModelLoader*>(parent->registry_ptr->FetchService(MODEL_LOADER));

        GB->clearFrame();

        for(int i = 0; i < actors.size(); i++) {
            IGraphicsBackend::placementData data = {
                .position = actors[i].worldPosition
            };
            int index = ML->getModelIndex(actors[i].modelHash);
            GB->addObjectToFrame(index, data);
        }

        return GB->drawFrame();
    }