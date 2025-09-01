#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IWindowManager : public InterfaceBaseClass {
    std::string token() override { return "IConfigManager"; }

    virtual ~IWindowManager() = default;
};

#endif