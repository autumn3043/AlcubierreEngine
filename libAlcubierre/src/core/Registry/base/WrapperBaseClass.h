#ifndef ALCENGINE_CORE_REGISTRY_BASE_WRAPPERBASECLASS_H
#define ALCENGINE_CORE_REGISTRY_BASE_WRAPPERBASECLASS_H

#include "core/Registry/base/GlobalDefinitions.h"
#include <unordered_map>

class InterfaceBaseClass;

class WrapperBaseClass {
    public:
        WrapperBaseClass() = default;
        virtual ~WrapperBaseClass() = default;

        std::unordered_map<int, InterfaceBaseClass*> Services;
};

#endif

