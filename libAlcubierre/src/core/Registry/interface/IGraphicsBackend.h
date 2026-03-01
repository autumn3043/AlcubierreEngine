#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H

#include "core/Registry/base/InterfaceBaseClass.h"

struct SceneObject {
    Hash_T meshHash;
    Vector3 position;
};

class IGraphicsBackend : public InterfaceBaseClass {
    public:
        std::string token() override { return "IGraphicsBackend"; }

        virtual int storeMesh(Hash_T hash, std::vector<Vector3>& vertices, std::vector<uint32_t>& indices) = 0;
        virtual int discardMesh(Hash_T hash) = 0;

        virtual int incrementMeshConsumers(Hash_T hash) = 0;
        virtual int decrementMeshConsumers(Hash_T hash) = 0;

        virtual int addToFrame(SceneObject object) = 0;
        virtual int clearFrame() = 0;
        virtual int drawFrame() = 0;
};

#endif