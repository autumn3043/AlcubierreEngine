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

        bool ShouldCloseImpl();

        void FrameImpl();

    private:
        Registry registry;
};

#endif