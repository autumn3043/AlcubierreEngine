#include "module/DiskInterface/private.h"

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "DiskInterface"), Write); }

ModuleRegistryBundle DiskInterface::bundle(
    [](void* registry) -> WrapperBaseClass* { return new DiskInterface(registry); },
    {DISK_INTERFACE},
    {},
    "DiskInterface"
);

DiskInterface::DiskInterface(void* registry) 
    :   IDiskInterface_DiskInterface(this), //Initialise service interface implementations in the constructor's initialiser list.
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{DISK_INTERFACE, &IDiskInterface_DiskInterface}}; //The module must then provide a pointer to the promised implementation once constructed.
        init();
    }

void DiskInterface::init() {}

#include <fstream>
std::string DiskInterface::read(std::string path) {
    std::ifstream file(path, std::ios::in);
    if(!file.is_open()) return "ERROR READING FILE '" + path + "'";
    std::string rawString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return rawString;
}

int DiskInterface::write(std::string path, std::string data) {
    std::ofstream file(path, std::ios::out);
    if(!file.is_open()) return 1;
    file << data << std::endl;
    file.close();
    return 0;
}

int DiskInterface::append(std::string path, std::string data, bool newline) {
    std::ofstream file(path, std::ios::app);
    if(!file.is_open()) return 1;
    file << data;
    if(newline) file << std::endl;
    file.close();
    return 0;
}