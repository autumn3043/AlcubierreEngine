#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWSURFACEBRIDGE_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWSURFACEBRIDGE_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IWindowSurfaceBridge : public InterfaceBaseClass {
    public:
        std::string token() override { return "IWindowSurfaceBridge"; }

        virtual int createWindowSurface(void* TargetInstance, void* TargetSurfaceObject) = 0;
};

#endif