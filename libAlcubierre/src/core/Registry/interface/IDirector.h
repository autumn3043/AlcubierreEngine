#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IDIRECTOR_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IDIRECTOR_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IDirector : public InterfaceBaseClass {
    public:
        std::string token() override { return "IDirector"; }

        virtual int createScene() = 0;
        virtual int createActor(Vector& worldPosition, std::vector<Vector>& modelVertices, std::vector<uint32_t>& modelIndices) = 0;
        virtual int renderScene() = 0;
};

#endif