#ifndef ALCENGINE_MODULE_DIRECTOR_H
#define ALCENGINE_MODULE_DIRECTOR_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IModelLoader.h"
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
        };
        IDirectorImpl IDirector_Director;

    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

    private:
        class WorldObject {
            public:
                uint64_t uniqueId;

                WorldObject(int _index);
                ~WorldObject();

                int assignMesh(uint32_t modelId);
                int moveto(Vector3 _position);

                Vector3 position = {0, 0, 0};
                uint32_t meshId;
        };

        class Scene {
            public:
                uint64_t uniqueId;

                Scene(int _memoryIndex);
                ~Scene();

                int addObject(WorldObject* object);
                int removeObject(WorldObject* object);

            private:
                int memoryIndex;

                std::unordered_map<uint32_t, std::unordered_map<uint64_t, WorldObject*>> buffers;
        };

        //These 32 bit integers will overflow back to 0 after 4294967295 objects have been created. If this happens, ids could clash. No guarantee id 0 won't exist anymore even after the lifetimes of some number of objects.
        //Update: made them 64 bit. Was concerned about reaching that object limit via particles or other dynamics. 
        //Non trivial as each id is copied x times, where x is the number of scenes the object is in, or the number of objects the scene contains. Making those ids references could help, but move constructors would complain.
        std::vector<WorldObject*> objects;
        uint64_t objectsEver = 0;

        std::vector<Scene*> scenes;
        uint64_t scenesEver = 0;

    public:
        int renderFrame();

        WorldObject* createObject();
        int destroyObject(WorldObject* object);

        Scene* createScene();
        int destroyScene(Scene* scene);
};

#endif