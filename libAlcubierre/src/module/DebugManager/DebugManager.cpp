#include "module/DebugManager/DebugManager.h"

#include <iostream>
#include <utility>
#include <string>

DebugManager& DebugManager::GetDebugManager() {
    static DebugManager instance;

    return instance;
}

//Public statics for shorthand interfacing

void DebugManager::Log(std::string Message, bool Write) { 
    DebugReport Report(Message);
    GetDebugManager().InternalLog(std::move(Report), Write);
}

void DebugManager::Log(std::exception exception, bool Write) {
    DebugReport Report(std::string(exception.what()));
    GetDebugManager().InternalLog(std::move(Report), Write);
} 

void DebugManager::Log(AlcExcept exception, bool Write) {
    GetDebugManager().InternalLog(std::move(exception.Get()), Write);
}

void DebugManager::Log(DebugReport Report, bool Write) {
    GetDebugManager().InternalLog(std::move(Report), Write);
}

//Private management of demystified input

void DebugManager::InternalLog(DebugReport Report, bool Write) {
    std::cout << Report.Message << std::endl;
}