#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IDIRECTOR_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IDIRECTOR_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IDirector : public InterfaceBaseClass {
    public:
        std::string token() override { return "IDirector"; }

        virtual int renderFrame() = 0;

        struct WorldObject_Handle { void* ptr = nullptr };
        virtual int createObject(WorldObject_Handle& object) = 0;
        virtual int destroyObject(WorldObject_Handle& object) = 0;
        virtual int attachMesh(WorldObject_Handle& object, uint32_t modelHash) = 0;
        virtual int setPosition(WorldObject_Handle& object, Vector3 newPosition) = 0;

        struct Scene_Handle { void* ptr = nullptr };
        virtual int createScene(Scene_Handle& scene) = 0;
        virtual int destroyScene(Scene_Handle& scene) = 0;
};

#endif