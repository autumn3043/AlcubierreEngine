#include "core/Registry/private.h"
#include "core/Registry/public.h"

#include "core/DebugManager/public.h"

#include <climits>

Registry::Registry() {
    PrivatePtr = new RegistryImpl(this);
}

Registry::~Registry() {
    delete PrivatePtr;
}

InterfaceBaseClass*& Registry::FetchService(ServiceID id) { 
    if(!PrivatePtr) throw AlcEngineException(std::string("Registry does not exist."));
    return PrivatePtr->FetchService(id); 
}

RegistryImpl::RegistryImpl(Registry* _parent) 
    : registry_ptr(_parent) {
    
    std::vector<ModuleRegistryBundle*>& modules = Registry::moduleQueueStaticAccessor();
    for(int i = 0; i < modules.size(); i++) {
        Modules.emplace_back(modules[i], registry_ptr);
        for(ServiceID id : modules[i]->Provides) {
            Services[id].first.push_back(i);
        }
    }
}

RegistryImpl::~RegistryImpl() {
    for(ModuleData& module : Modules) {
        GracefulDeconstruct(module);
    }
}

int RegistryImpl::GracefulDeconstruct(ModuleData& module) {
    if(module.exists()) {
        for(const ServiceID& id : module.serviceArr()) {
            for(int& dependent : Services[id].second) {
                GracefulDeconstruct(Modules[dependent]);
            }
        }

        module.destruct();
        return 0;
    }
    return 1;
}

InterfaceBaseClass*& RegistryImpl::FetchService(ServiceID id) {
    if(!Services.contains(id)) throw AlcEngineException("Requested service '" + ServiceID_str(id) + "' does not exist.");
    if(Services[id].first.size() == 0) throw AlcEngineException("Requested service '" + ServiceID_str(id) + "' is not provided by any module.");
    
    int provider_index = SelectBestProvider(id);
    ModuleData& provider = Modules[provider_index];

    if(!provider.exists()) {
        provider.instantiate();

        //When we create a module, we add a reference to it to the entries of each of its dependencies. When deconstructing said dependencies we then know to deconstruct this module first
        for(const ServiceID& id : provider.dependsArr()) {
            Services[id].second.push_back(provider_index);
        }
    }

    return provider.GetService(id);
}

int RegistryImpl::SelectBestProvider(ServiceID id) {
    //From amongst many modules which may provide a variable number of services, select the best option
    //Priority is initialising the minimum number of modules possible into memory, so we prefer providers which are already filling another purpose
    //TODO allow modules to lockout a service if they are chosen to provide it - i.e. configuration_manager service needs to be the same module for all dependents, cannot change based on circumstance 

    int bestScore = INT_MIN;
    int bestModule_index = -1;

    for(int provider_index : Services[id].first) {
        ModuleData& provider = Modules[provider_index];
        int score = 0;

        if(provider.exists()) score += 10;
        score += provider.serviceArr().size();
        score -= provider.dependsArr().size();

        if(score > bestScore) {
            bestScore = score;
            bestModule_index = provider_index;
        }
    }

    return bestModule_index;
}

RegistryImpl::ModuleData::ModuleData(ModuleRegistryBundle* _bundle, Registry* registry)
    :   Instance(nullptr),
        registry_ptr(registry),
        Label(_bundle->Label),
        Constructor(_bundle->Constructor),
        Provides(_bundle->Provides), 
        Depends(_bundle->Depends) 
    {}

RegistryImpl::ModuleData::~ModuleData() {
    if(exists()) destruct();
}

bool RegistryImpl::ModuleData::exists() { return Instance != nullptr; }
int RegistryImpl::ModuleData::instantiate() { 
    if(exists()) destruct();
    DM().Log("Instantiating module: " + Label);
    Instance = std::unique_ptr<WrapperBaseClass>(Constructor(registry_ptr));
    return 0;
}
int RegistryImpl::ModuleData::destruct() { 
    Instance.reset();
    DM().Log("Destructed module: " + Label);
    return 0;
}
const std::vector<ServiceID>& RegistryImpl::ModuleData::serviceArr() { return Provides; }
const std::vector<ServiceID>& RegistryImpl::ModuleData::dependsArr() { return Depends; }
InterfaceBaseClass*& RegistryImpl::ModuleData::GetService(ServiceID id) { return Instance->Services[id]; }

ModuleRegistryBundle::ModuleRegistryBundle(std::function<WrapperBaseClass*(void*)> _constructor, std::vector<ServiceID> _provides, std::vector<ServiceID> _depends, std::string _label) {
    Constructor = _constructor;
    Provides = _provides;
    Depends = _depends;
    Label = _label;

    Registry::moduleQueueStaticAccessor().push_back(this);
}