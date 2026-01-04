#ifndef ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H
#define ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H

#include "core/Registry/base/GlobalDefinitions.h"

class InterfaceBaseClass {
    public:
        virtual std::string token() = 0;

        virtual ~InterfaceBaseClass() = default;
};

#endif
