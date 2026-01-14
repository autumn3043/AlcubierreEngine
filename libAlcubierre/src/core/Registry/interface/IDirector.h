#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IDIRECTOR_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IDIRECTOR_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IDirector : public InterfaceBaseClass {
    public:
        std::string token() override { return "IDirector"; }

        virtual int createScene() = 0;
        virtual int createActor(Vector& worldPosition, std::string& model) = 0;
        virtual int renderScene() = 0;
};

#endif