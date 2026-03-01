#include "module/ResourceManager/private.h"

#include "extern/ModelPrecompiler/mesh_def.h"

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
    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));
    for(int i = 0; i < modelsInMemory.size(); i++) {
        GB->discardMesh(modelsInMemory[i]);
    }
}

int ResourceManager::init() {
    return 0;
}

Hash_T ResourceManager::load(char* model, uint64_t size) {
    std::vector<Vector3> modelVertices;
    std::vector<uint32_t> modelIndices;

    AlcubierreEngineMesh modelData;
    // logIdentity(std::to_string(size));
    int result = modelData.deserialise(model, size);
    if(result != 0) throw ResourceManagerException("Failed to deserialise a model");

    std::vector<Vector3> vertices(modelData.body.vertexCount);
    memcpy(vertices.data(), modelData.body.vertices, modelData.header.vertexSize * modelData.body.vertexCount);
    std::vector<uint32_t> indices(modelData.body.indexCount);
    memcpy(indices.data(), modelData.body.indices, modelData.header.indexSize * modelData.body.indexCount);

    modelsInMemory.emplace_back(modelData.header.hash);
    dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND))->storeMesh(modelData.header.hash, vertices, indices);

    return modelData.header.hash;
}