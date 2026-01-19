#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_DISK_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_DISK_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IDiskInterface : public InterfaceBaseClass {
    public:
        std::string token() override { return "IDiskInterface"; }

        virtual std::string read(std::string path) = 0;
        virtual int write(std::string path, std::string data) = 0;
        virtual int append(std::string path, std::string data, bool newline = true) = 0;
};

#endif