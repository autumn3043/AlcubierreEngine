#include "core/PublicInterface/private.h"
#include "core/PublicInterface/public.h"

#include "core/DebugManager/public.h"

AlcubierreEngine::AlcubierreEngine() {
    PrivatePtr = new AlcubierreEngineImpl();
}

AlcubierreEngine::~AlcubierreEngine() {
    delete PrivatePtr;
}

AlcubierreEngineImpl::AlcubierreEngineImpl() {
    Registry::GetRegistry().Init();
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    CM->Set<std::string>(R"({
    "name": "Autumn",
    "age": 18,
    "is_developer": true,
    "skills": ["C++", "Vulkan", "JSON"],
    "details": {
        "location": "Australia",
        "active": true
    }
    })"
    );

    DM().Log(CM->Get<std::string>("name", "default"));

    std::vector<std::string> skills = CM->Get<std::vector<std::string>>("skills", {"test1", "test2", "test3"});
    for(std::string& skill : skills) {
        DM().Log(skill);
    }
}

AlcubierreEngineImpl::~AlcubierreEngineImpl() {
    
}