#include "core/Registry/private.h"
#include "core/Registry/public.h"

Registry::Registry() {
    PrivatePtr = new RegistryImpl();
}

Registry& Registry::GetRegistry() {
    static Registry instance;

    return instance;
}

void Registry::RegisterModule(std::function<WrapperBaseClass*()> _constructor) {
    PrivatePtr->Constructors.push_back(_constructor);
}

void Registry::Init() {
    for (std::function<WrapperBaseClass*()> constructor : PrivatePtr->Constructors) {
        PrivatePtr->Modules.emplace_back(constructor());
    }
}

void Registry::RegisterService(InterfaceBaseClass& _service) {
    PrivatePtr->Services.insert(std::make_pair(_service.token, _service));
}

InterfaceBaseClass& Registry::FetchService(std::string token) {
    return PrivatePtr->Services.find(token)->second;
}