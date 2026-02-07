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

    for(int i = 0; i < scenes.size(); i++) {
        for(iterator j = scenes[i].buffers.begin(); j != scenes[i].buffers.end(); j++) {
            GB->addToFrame(j->first());
        }
    }

    GB->drawFrame();
    GB->clearBufferSets();
}

WorldObject* Director::createObject() {
    objects.push_back(new WorldObject(objectsEver, objects.size()));
    objectsEver++;
    return objects.back();
}

int Director::destroyObject(WorldObject* object) {
    for(iterator i = scenes.begin(); i != scenes.end(); i++) {
        i->second()->removeObject(this);
    }

    if(object->memoryIndex != objects.size() - 1) {
        std::swap(objects[object->memoryIndex], objects.back());
        objects[object->memoryIndex]->memoryIndex = object->memoryIndex;
    }

    delete object;
    objects.pop_back();

    return 0;
}

Director::WorldObject::WorldObject(uint64_t _uniqueId, int _memoryIndex) : uniqueId(_uniqueId), memoryIndex(_memoryIndex) {}

Director::WorldObject::~WorldObject() {
}

int Director::WorldObject::assignMesh(uint32_t modelId) {
    meshId = modelId;
}

int Director::WorldObject::moveto(Vector _position) {
    position = _position;
}

Scene* Director::createScene() {
    scenes.push_back(new Scene(scenesEver, scenes.size()));
    scenesEver++;
    return scenes.back();
}

int Director::destroyScene(Scene* scene) {
    for(iterator i = scene->buffers.begin() - 1; i != scene->buffers.end(); i++) {
        std::unordered_map<uint32_t, WorldObject*> x = i->second();
        for(iterator j = x->begin() - 1; j != x->end(); j++) {
            j->second()->scenes.erase(uniqueId);
        }
        assert(x->empty());
    }

    if(scene->memoryIndex != scenes.size() - 1) {
        std::swap(scenes[scene->memoryIndex], scenes.back());
        scenes[scene->memoryIndex]->memoryIndex = scene->memoryIndex;
    }

    delete scene;
    scenes.pop_back();

    return 0;
}

Director::Scene::Scene(uint64_t _uniqueId, int _memoryIndex) : uniqueId(_uniqueId), memoryIndex(_memoryIndex) {}
Director::Scene::~Scene() {}

int Scene::addObject(WorldObject* object) {
    uint32_t& bufferId = object->mesh->bufferId;
    if(buffers.contains(bufferId) && buffers[bufferId].contains(object->uniqueId)) return 1; //Already in that scene.
    if(!buffers.contains(bufferId)) buffers.emplace(bufferId, std::unordered_map<uint32_t, WorldObject*>());
    buffers[bufferId].emplace(object->uniqueId, object);
    object->scenes.emplace(uniqueId, this);
    return 0;
}

int Scene::removeObject(WorldObject* object) {
    uint32_t& bufferId = object->mesh->bufferId;
    if(!buffers.contains(bufferId)) return 2; //That object is loaded to a buffer which is not included in this scene.
    if(!buffers[bufferId].contains(object->uniqueId)) return 1; //That object is not included in this scene.
    buffers[bufferId].erase(object->uniqueId);
    if(buffers[bufferId].empty()) buffers.erase(bufferId);
    object->scenes.erase(uniqueId);
    return 0;
}

int IDirectorImpl::createObject(WorldObject_Handle& object) override {
    object.ptr = static_cast<void*>(parent->createObject());
    return 0;
}

int IDirectorImpl::destroyObject(WorldObject_Handle& object) override {
    int r = parent->destroyObject(static_cast<WorldObject*>(object.ptr));
    object.ptr = nullptr;
    return r;
}

int IDirectorImpl::attachMesh(WorldObject_Handle& object, uint32_t modelHash) override {
    return static_cast<WorldObject*>(object.ptr)->assignMesh(modelHash);
}

int IDirectorImpl::setPosition(WorldObject_Handle& object, Vector newPosition) {
    return static_cast<WorldObject*>(object.ptr)->moveto(newPosition);
}

int IDirectorImpl::createScene(Scene_Handle& scene) override {
    scene.ptr = static_cast<void*>(parent->createScene());
    return 0;
}

int IDirectorImpl::destroyScene(Scene_Handle& scene) override {
    int r = parent->destroyScene(static_cast<Scene*>(scene.ptr));
    scene.ptr = nullptr;
    return r;
}