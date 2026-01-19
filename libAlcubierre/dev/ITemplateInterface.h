#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE__EXAMPLE_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE__EXAMPLE_H

#include "core/Registry/base/InterfaceBaseClass.h"

class I_example : public InterfaceBaseClass {
    public:
        std::string token() override { return "I_example"; }
};

#endif