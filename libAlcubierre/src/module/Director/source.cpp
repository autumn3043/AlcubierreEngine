#include "module/Director/private.h"

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "Director"), Write); }

ModuleRegistryBundle Director::bundle(
    [](void* registry) -> WrapperBaseClass* { return new Director(registry); },
    {DIRECTOR},
    {MODEL_LOADER},
    "Director"
);

Director::Director(void* registry) 
    :   IDirector_Director(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{DIRECTOR, &IDirector_Director}};
        init();
    }

void Director::init() {}

int Director::renderFrame() {
    IGraphicsBackend* GB = dynamic_cast<IGraphicsBackend*>(registry_ptr->FetchService(GRAPHICS_BACKEND));

    for(std::unordered_map<uint64_t, Scene>::iterator it = scenes.begin(); it != scenes.end(); it++) {
        for(int j = 0; j < it->second.sceneObjectIds.size(); j++) {
            WorldObject& object = objects.at(it->second.sceneObjectIds[j]);
            if(!object.hasMesh) continue;
            GB->addToFrame({object.meshHash, object.position});
        }
    }

    GB->drawFrame();
    GB->clearFrame();

    return 0;
}

uint64_t* Director::createObjectImpl() {
    uint64_t* r = &objects.emplace(objectsEver, WorldObject(objectsEver)).first->second.uniqueId;
    objectsEver++;
    return r;
}

int Director::destroyObjectImpl(uint64_t* objectId) {
    objects.erase(*objectId);

    return 0;
}

int Director::attachMeshImpl(uint64_t* objectId, uint32_t meshHash) {
    objects.at(*objectId).attachMesh(meshHash);

    return 0;
}

int Director::setPositionImpl(uint64_t* objectId, Vector3 newPosition) {
    objects.at(*objectId).moveto(newPosition);

    return 0;
}

Director::WorldObject::WorldObject(uint64_t _uniqueId) : uniqueId(_uniqueId) {}
Director::WorldObject::~WorldObject() {}

int Director::WorldObject::attachMesh(uint32_t _meshHash) {
    meshHash = _meshHash;
    hasMesh = true;

    return 0;
}

int Director::WorldObject::moveto(Vector3 _position) {
    position = _position;

    return 0;
}

uint64_t* Director::createSceneImpl() {
    uint64_t* r = &scenes.emplace(scenesEver, Scene(scenesEver)).first->second.uniqueId;
    scenesEver++;
    return r;
}

int Director::destroySceneImpl(uint64_t* sceneId) {
    scenes.erase(*sceneId);

    return 0;
}

int Director::addToSceneImpl(uint64_t* sceneId, uint64_t* objectId) {
    scenes.at(*sceneId).addObject(objectId);

    return 0;
}

Director::Scene::Scene(uint64_t _uniqueId) : uniqueId(_uniqueId) {}
Director::Scene::~Scene() {}

int Director::Scene::addObject(uint64_t* objectId) {
    sceneObjectIds.emplace_back(*objectId);

    return 0;
}