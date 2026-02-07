#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H

#include "core/Registry/base/InterfaceBaseClass.h"

struct Mesh3D {
    uint32_t bufferSetId;

    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct SceneObject {
    Mesh3D* mesh = nullptr;

};

class IGraphicsBackend : public InterfaceBaseClass {
    public:
        std::string token() override { return "IGraphicsBackend"; }

        virtual int addToFrame(SceneObject* object) = 0;
        virtual int clearFrame() = 0;
        virtual int drawFrame() = 0;
};

#endif