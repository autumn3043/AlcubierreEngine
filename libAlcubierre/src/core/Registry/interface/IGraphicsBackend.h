#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IGraphicsBackend : public InterfaceBaseClass {
    public:
        std::string token() override { return "IGraphicsBackend"; }

        virtual void* GetBackendObject() = 0;
};

#endif