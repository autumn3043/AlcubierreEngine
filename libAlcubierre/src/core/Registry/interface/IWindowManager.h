#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IWINDOWMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IWindowManager : public InterfaceBaseClass {
    public:
        std::string token() override { return "IWindowManager"; }

        virtual void* GetWindowObject() = 0;

        struct WindowInfo {
            std::string name;
            int width;
            int height;
            int width_pix;
            int height_pix;
        };

        virtual WindowInfo* GetWindowInfo() = 0;

        virtual bool TouchSurfaceApi() = 0;
};

#endif