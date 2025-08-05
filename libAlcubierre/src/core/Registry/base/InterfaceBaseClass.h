#ifndef ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H
#define ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H

#include <string>
#include <functional>

class InterfaceBaseClass {
    public:
        std::string token;

        virtual ~InterfaceBaseClass() = default;
};

#endif
