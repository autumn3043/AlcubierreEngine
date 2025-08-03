#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IConfigManager : public InterfaceBaseClass {
    std::string token override = "IConfigManager";

    template <typename T>
    std::function<T(std::string)> Get;
};

#endif