#ifndef ALCENGINE_MODULE__example_H
#define ALCENGINE_MODULE__example_H

#include "core/Registry/public.h"

//Services
//Depends
#include "core/Registry/interface/IExampleService.h"
//Provides
#include "core/Registry/interface/IDifferentExampleService.h"
    //Modules will only be constructed if the registry determines that it is the most suitable provider of a requested service. They will then persist for the remaining lifetime of the application.
    //The first time a service is requested, the selection is dynamic, but that selection persists for the lifetime of the application, even if an alternative is or becomes available.

#include "core/DebugManager/public.h"

class _exampleException : public AlcEngineException {
    public:
        _exampleException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class _example : public WrapperBaseClass {
    public:
        _example(void* registry); //The constructor expected by the registry must be of this signature.

        void init(); //Your actual init function which may be of any signature you like.

        class IDifferentExampleServiceImpl : public IDifferentExampleService {
            private:
                _example* parent;

            public:
                IDifferentExampleServiceImpl(_example* _parent) : parent(_parent) {}
                
            //You may build the implementation directly here or point to methods on the parent class. 
        };
        IDifferentExampleServiceImpl IDifferentExampleService__example;
    
    private:
        static ModuleRegistryBundle bundle; //This statically constructed struct passes a pointer to the constructor of this class to the registry at pre-init.
        Registry* registry_ptr = nullptr;
};

#endif