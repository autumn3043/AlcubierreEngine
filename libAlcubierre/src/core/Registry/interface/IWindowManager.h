#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IWindowManager : public InterfaceBaseClass {
    public:
        std::string token() override { return "IWindowManager"; }

        virtual void* getWindowObject() = 0;
        struct WindowInfo {
            int width;
            int height;
            int width_pix;
            int height_pix;
        };
        virtual WindowInfo getWindowInfo() = 0;
        virtual void pollEvents() = 0;
        virtual bool shouldClose() = 0;


};

#endif