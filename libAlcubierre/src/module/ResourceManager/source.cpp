#include "module/ResourceManager/private.h"

#include "tiny_obj_loader.h"

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "ResourceManager"), Write); }

ModuleRegistryBundle ResourceManager::bundle(
    [](void* registry) -> WrapperBaseClass* { return new ResourceManager(registry); },
    {MODEL_LOADER},
    {GRAPHICS_BACKEND},
    "ResourceManager"
);

ResourceManager::ResourceManager(void* registry) 
    :   IModelLoader_ResourceManager(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{MODEL_LOADER, &IModelLoader_ResourceManager}};
        init();
    }

ResourceManager::~ResourceManager() {
    
}

Mesh3D* IModelLoader::loadModel(uint32_t& modelHash, rawModelData& model) override {
    if(!parent->isLoaded(modelHash)) parent->load(modelHash, model);
    return parent->getMesh(modelHash);
} 

Mesh3D* IModelLoader::fetchModel(uint32_t& modelHash) override {
    if(!parent->isLoaded(modelHash)) throw ResourceManagerException("Requested mesh was not loaded");
    return parent->getMesh(modelHash);
}

int ResourceManager::init() {}

bool ResourceManager::isLoaded(uint32_t& modelHash) {
    return modelsInMemory.contains(modelHash);
}

Mesh3D* ResourceManager::load(uint32_t& modelHash, rawModelData& modelData) {
    std::vector<Vector3> modelVertices;
    std::vector<uint32_t> modelIndices;

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig readerConfig;
    std::string modelAsString = std::string(reinterpret_cast<const char*>(modelData.data), modelData.size);
    reader.ParseFromString(modelAsString, std::string(), readerConfig);

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

            modelVertices.push_back(x, y, z);
        }
    }

    //Temp: obj file precompiler
        for(int i = 0; i < modelVertices.size(); i++) {
            modelIndices.push_back(i);
        }

    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));
    modelsInMemory.emplace(modelHash, {.vertexCount = modelVertices.size(), .indiceCount = modelIndices.size()});
    GB->loadMeshToBuffer(&modelsInMemory.back(), modelVertices, modelIndices);
    return &modelsInMemory.back();
}

Mesh3D* ResourceManager::getMesh(uint32_t modelHash) {
    return modelsInMemory[modelHash];
}