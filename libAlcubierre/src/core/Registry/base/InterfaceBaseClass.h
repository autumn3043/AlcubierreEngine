#ifndef ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H
#define ALCENGINE_CORE_REGISTRY_BASE_INTERFACEBASECLASS_H

#include <string>
#include <functional>

class InterfaceBaseClass {
    public:
        virtual InterfaceBaseClass() = 0; 
        //Override this in deriving classes to assign const references ro functions if you know what I mean

        virtual std::string token();
};

#endif
