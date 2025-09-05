#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IWindowManager : public InterfaceBaseClass {
    public:
        std::string token() override { return "IWindowManager"; }

        virtual void* GetWindowObject() = 0;
};

#endif