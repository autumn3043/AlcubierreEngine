#include "core/Registry/private.h"
#include "core/Registry/public.h"

#include "core/DebugManager/public.h"

Registry& Registry::GetRegistry() {
    static Registry instance;

    return instance;
}

Registry::Registry() {
    PrivatePtr = new RegistryImpl();
}

Registry::~Registry() {
    delete PrivatePtr;
}

void Registry::RegisterModule(std::function<WrapperBaseClass*()> _constructor, std::string& moduleID) {
    PrivatePtr->Constructors.push_back(_constructor);
    DM().Log("Registered module '" + moduleID + "'");
}

void Registry::Init() {
    for (std::function<WrapperBaseClass*()>& constructor : PrivatePtr->Constructors) {
        PrivatePtr->Modules.emplace_back(std::unique_ptr<WrapperBaseClass>(constructor()));
    }
}

void Registry::RegisterService(InterfaceBaseClass& _service) {
    PrivatePtr->Services[_service.token()] = &_service;
    DM().Log("Registered service '" + _service.token() + "'");
}

InterfaceBaseClass* Registry::FetchService(std::string token) {
    return PrivatePtr->Services.at(token);
}