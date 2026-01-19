#include "module/_example/private.h"

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "_example"), Write); } //Not strictly necessary, but convenient.

ModuleRegistryBundle _example::bundle(
    [](void* registry) -> WrapperBaseClass* { return new _example(registry); },
    {EXAMPLE_SERVICE}, //Provides
    {EXAMPLE_SERVICE2}, //*STRICT* dependencies. Services specified here will not be deconstructed until this module has been deconstructed. Should only be used if the service is accessed during teardown to prevent UB.
    "_example" //Internal label for registry debug
);

_example::_example(void* registry) 
    :   IDifferentExampleService__example(this), //Initialise service interface implementations in the constructor's initialiser list.
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{EXAMPLE_SERVICE, &IDifferentExampleService__example}}; //The module must then provide a pointer to the promised implementation once constructed.
        init();
    }

void _example::init() {}