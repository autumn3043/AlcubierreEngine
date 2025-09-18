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

RegistryImpl::~RegistryImpl() {
    const std::vector<std::string> DeconstructOrder = {
        "MODULE_VULKANHANDLER",
        "MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE",
        "MODULE_GLFWHANDLER",
        "MODULE_CONFIGMANAGER"
    };

    for (int i = 0; i < DeconstructOrder.size(); i++) {
        if(Modules.contains(DeconstructOrder[i])) {
            Modules.at(DeconstructOrder[i]).reset();
        }
    }
}

void Registry::RegisterModule(std::function<WrapperBaseClass*()> _constructor, const std::string& moduleID) {
    PrivatePtr->Constructors[moduleID] = std::move(_constructor);
    DM().Log("Registered module '" + moduleID + "'");
}

void Registry::Init() {
    for (std::unordered_map<std::string, std::function<WrapperBaseClass*()>>::iterator it = PrivatePtr->Constructors.begin(); it != PrivatePtr->Constructors.end(); it++) {
        const std::string& id = it->first;
        std::function<WrapperBaseClass*()>& constructor = it->second;

        PrivatePtr->Modules[id] = std::unique_ptr<WrapperBaseClass>(constructor()); // RegisterService is called here
    }
}

void Registry::RegisterService(InterfaceBaseClass& _service) {
    if(!PrivatePtr->Services.contains(_service.token())) {
        PrivatePtr->Services[_service.token()] = &_service;
        DM().Log("Registered service '" + _service.token() + "'");
    } else {
        DM().Log("Failed attempt to register service with non-unique token '" + _service.token() + "'");
    }
}

InterfaceBaseClass* Registry::FetchService(std::string token) {
    if(PrivatePtr->Services.contains(token)) {
        PrivatePtr->Services.at(token)->Wake();
        return PrivatePtr->Services.at(token);
    } else {
        throw AlcEngineException(DebugReport("Attempted to access unregistered service '" + token + "'"));
    }
}