#ifndef ALCENGINE_MODULE_DIRECTOR_H
#define ALCENGINE_MODULE_DIRECTOR_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IGraphicsBackend.h"
//Provides
#include "core/Registry/interface/IDirector.h"

#include "core/DebugManager/public.h"

class DirectorException : public AlcEngineException {
    public:
        DirectorException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class Director : public WrapperBaseClass {
    public:
        Director(void* registry);

        void init();

        class IDirectorImpl : public IDirector {
            private:
                Director* parent;

            public:
                IDirectorImpl(Director* _parent) : parent(_parent) {}

                int renderFrame() override { return parent->renderFrame(); }

                int createObject(WorldObject_Handle& object) override { return parent->createObject(object); }
                int destroyObject(WorldObject_Handle& object) override { return parent->destroyObject(object); }
                int attachMesh(WorldObject_Handle& object, Hash_T meshHash) override { return parent->attachMesh(object, meshHash); }
                int detachMesh(WorldObject_Handle& object) override { return parent->detachMesh(object); }
                int setPosition(WorldObject_Handle& object, Vector3 newPosition) override { return parent->setPosition(object, newPosition); }

                int createScene(Scene_Handle& scene) override { return parent->createScene(scene); }
                int destroyScene(Scene_Handle& scene) override { return parent->destroyScene(scene); }
                int addToScene(Scene_Handle& scene, WorldObject_Handle& object) override { return parent->addToScene(scene, object); } 
        };
        IDirectorImpl IDirector_Director;

    private:
        static ModuleRegistryBundle bundle;

    protected:
        Registry* registry_ptr = nullptr;

    private:
        class WorldObject {
            public:
                uint64_t uniqueId;

                WorldObject(Director* _parent, uint64_t _uniqueId);
                ~WorldObject();

                Director* parent;

                int attachMesh(Hash_T meshHash);
                int detachMesh();
                int moveto(Vector3 _position);

                Vector3 position = {0, 0, 0};

                bool hasMesh = false;
                Hash_T meshHash;
        };

        class Scene {
            public:
                uint64_t uniqueId;

                Scene(uint64_t _uniqueId);
                ~Scene();

                int addObject(uint64_t* object);

                std::vector<uint64_t> sceneObjectIds;
        };

        std::unordered_map<uint64_t, WorldObject> objects;
        uint64_t objectsEver = 0;
        std::unordered_map<uint64_t, Scene> scenes;
        uint64_t scenesEver = 0;

    public:
        int renderFrame();

        int createObject(IDirector::WorldObject_Handle& object) { object.ptr = createObjectImpl(); return 0; }
        int destroyObject(IDirector::WorldObject_Handle& object) { return destroyObjectImpl(static_cast<uint64_t*>(object.ptr)); }
        int attachMesh(IDirector::WorldObject_Handle& object, Hash_T meshHash) { return attachMeshImpl(static_cast<uint64_t*>(object.ptr), meshHash); }
        int detachMesh(IDirector::WorldObject_Handle& object) { return detachMeshImpl(static_cast<uint64_t*>(object.ptr)); }
        int setPosition(IDirector::WorldObject_Handle& object, Vector3 newPosition) { return setPositionImpl(static_cast<uint64_t*>(object.ptr), newPosition); }

        int createScene(IDirector::Scene_Handle& scene) { scene.ptr = createSceneImpl(); return 0; }
        int destroyScene(IDirector::Scene_Handle& scene) { return destroySceneImpl(static_cast<uint64_t*>(scene.ptr)); }
        int addToScene(IDirector::Scene_Handle& scene, IDirector::WorldObject_Handle& object) { return addToSceneImpl(static_cast<uint64_t*>(scene.ptr), static_cast<uint64_t*>(object.ptr)); }

    private:
        uint64_t* createObjectImpl();
        int destroyObjectImpl(uint64_t* objectId);
        int attachMeshImpl(uint64_t* objectId, Hash_T meshHash);
        int detachMeshImpl(uint64_t* objectId);
        int setPositionImpl(uint64_t* objectId, Vector3 newPosition);

        uint64_t* createSceneImpl();
        int destroySceneImpl(uint64_t* sceneId);
        int addToSceneImpl(uint64_t* sceneId, uint64_t* objectId);
};

#endif