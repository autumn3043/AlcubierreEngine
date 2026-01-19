#ifndef ALCENGINE_MODULE_disk_H
#define ALCENGINE_MODULE_disk_H

#include "core/Registry/public.h"

//Services
//Depends
//Provides
#include "core/Registry/interface/IDiskInterface.h"

#include "core/DebugManager/public.h"

class DiskInterfaceException : public AlcEngineException {
    public:
        DiskInterfaceException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class DiskInterface : public WrapperBaseClass {
    public:
        DiskInterface(void* registry);

        void init();

        class IDiskInterfaceImpl : public IDiskInterface {
            private:
                DiskInterface* parent;

            public:
                IDiskInterfaceImpl(DiskInterface* _parent) : parent(_parent) {}

                std::string read(std::string path) override { return parent->read(path); }
                int write(std::string path, std::string data) override { return parent->write(path, data); }
                int append(std::string path, std::string data, bool newline) override { return parent->append(path, data, newline); }
        };
        IDiskInterfaceImpl IDiskInterface_DiskInterface;

        std::string read(std::string path);
        int write(std::string path, std::string data); 
        int append(std::string path, std::string data, bool newline);
    
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;
};

#endif