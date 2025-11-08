#ifndef ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H
#define ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H

#include <string>
#include <functional>

class InterfaceBaseClass {
    public:
        virtual std::string token() = 0;

        virtual ~InterfaceBaseClass() = default;
};

#endif
