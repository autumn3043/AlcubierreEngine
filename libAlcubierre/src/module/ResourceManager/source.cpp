#include "module/ResourceManager/private.h"

#include "extern/ModelPrecompiler/public.h"

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
    Mesh modelData(model, size);

    std::vector<Vertex> vertices(modelData.verticeCount);
    memcpy(vertices.data(), modelData.vertices, sizeof(Vertex) * modelData.verticeCount);
    std::vector<uint32_t> indices;
    for(int i = 0; i < modelData.subMeshCount; i++) {
        int oldSize = indices.size();
        indices.resize(oldSize + modelData.subMeshes[i].indiceCount);
        memcpy(indices.data() + oldSize, modelData.subMeshes[i].indices, sizeof(uint32_t) * modelData.subMeshes[i].indiceCount);
    }

    modelsInMemory.emplace_back(modelData.header.hash);
    dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND))->storeMesh(modelData.header.hash, vertices, indices);

    return modelData.header.hash;
}