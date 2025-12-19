#ifndef ALCENGINE_PUBLICINTERFACE_PRIVATE_H
#define ALCENGINE_PUBLICINTERFACE_PRIVATE_H

#include "core/Registry/public.h"

//Services
#include "core/Registry/interface/IWindowSurfaceBridge.h"
#include "core/Registry/interface/IConfigManager.h"
#include "core/Registry/interface/IWindowManager.h"
#include "core/Registry/interface/IGraphicsBackend.h"

class AlcubierreEngineImpl {
    private:
        Registry registryMasterInstance;

        const std::vector<int> ENGINE_VERSION = {0, 0, 0};

    public:
        AlcubierreEngineImpl();
        ~AlcubierreEngineImpl();
        void initEngine();

        //Debug
            void log(std::string& message, int& priority);

        //Config
            template <typename T>
            int get(std::vector<std::string>& key, T*& returnPtr) {
                int result;
                *returnPtr = dynamic_cast<IConfigManager*>(registryMasterInstance.FetchService(CONFIGURATION_MANAGER))->Get<T>(key, &result);
                return result;
            }
            int parse(std::string& value);
            int dump();

        //Window
            bool shouldClose();

        //Graphics
            int frame();

        //Input
};

#endif