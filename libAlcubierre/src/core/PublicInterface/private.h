#ifndef ALCENGINE_PUBLICINTERFACE_PRIVATE_H
#define ALCENGINE_PUBLICINTERFACE_PRIVATE_H

#include "core/Registry/public.h"

//Services
#include "core/Registry/interface/IWindowSurfaceBridge.h"
#include "core/Registry/interface/IConfigManager.h"
#include "core/Registry/interface/IWindowManager.h"
#include "core/Registry/interface/IGraphicsBackend.h"

class AlcubierreEngineImpl {
    public:
        AlcubierreEngineImpl();
        ~AlcubierreEngineImpl();
        void InitEngineImpl();

        bool ShouldCloseImpl();

        void FrameImpl();

        int SetConfigFromJsonStringImpl(std::string jsonString);

    private:
        Registry registry_ptr;

        const std::vector<int> ENGINE_VERSION = {0, 0, 0};
};

#endif