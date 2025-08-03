#ifndef ALCENGINE_CORE_REGISTRY_PUBLIC_H
#define ALCENGINE_CORE_REGISTRY_PUBLIC_H

#include <functional>
#include <string>

#include "core/Registry/base/WrapperBaseClass.h"
#include "core/Registry/base/InterfaceBaseClass.h"

class RegistryImpl;

class Registry {
    public:
        static Registry& GetRegistry();

        void RegisterModule(std::function<WrapperBaseClass*()> _constructor); //Get the constructor, add it to a list...
        void Init(); //...and fire them.
        void RegisterService(InterfaceBaseClass& _service);
        InterfaceBaseClass& FetchService(std::string token);

    private:
        Registry();
        ~Registry();

        RegistryImpl* PrivatePtr;
};

struct ModuleRegistryBundle {
    std::function<WrapperBaseClass*()> constructor;

    ModuleRegistryBundle(std::function<WrapperBaseClass*()> _constructor) : constructor(std::move(_constructor)) {
        Registry::GetRegistry().RegisterModule(constructor);
    }
};

#endif