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
    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));
    for(int i = 0; i < modelsInMemory.size(); i++) {
        GB->discardMesh(modelsInMemory[i]);
    }
}

int ResourceManager::init() {
    return 0;
}

int ResourceManager::load(IModelLoader::rawModelData& modelData) {
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

            modelVertices.emplace_back(x, y, z);
        }
    }

    //Temp: obj file precompiler
        for(int i = 0; i < modelVertices.size(); i++) {
            modelIndices.emplace_back(i);
        }

    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));
    modelsInMemory.emplace_back(modelData.hash);
    GB->storeMesh(modelData.hash, modelVertices, modelIndices);
    return 0;
}