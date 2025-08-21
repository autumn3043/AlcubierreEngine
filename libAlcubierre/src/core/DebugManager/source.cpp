#include "core/DebugManager/public.h"

#include <iostream>
#include <utility>
#include <string>
#include <sstream>
#include <iomanip>

DebugManager& DebugManager::GetDebugManager() {
    static DebugManager Instance;
    return Instance;
}

DebugManager::DebugManager() {}

DebugManager::~DebugManager() {}

// Public static methods for shorthand interfacing

void DebugManager::Log(std::string Message, bool Write) { 
    DebugReport Report(Message);
    DebugManager::GetDebugManager().InternalLog(std::move(Report), Write);
}

void DebugManager::Log(std::exception exception, bool Write) {
    DebugReport Report(std::string(exception.what()));
    DebugManager::GetDebugManager().InternalLog(std::move(Report), Write);
} 

void DebugManager::Log(AlcEngineException exception, bool Write) {
    DebugManager::GetDebugManager().InternalLog(std::move(exception.Get()), Write);
}

void DebugManager::Log(DebugReport Report, bool Write) {
    DebugManager::GetDebugManager().InternalLog(std::move(Report), Write);
}

//Private management of demystified inpui

void DebugManager::InternalLog(DebugReport Report, bool Write) {

    tm _time = *localtime(&Report.Time);
    std::ostringstream oss;
    oss << "@["
        << std::setw(2) << std::setfill('0') << _time.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << _time.tm_min << ":"
        << std::setw(2) << std::setfill('0') << _time.tm_sec << "]: ";

    std::string timeHold = oss.str();

    std::string invokeHold = "";
    if(Report.Invoker != "") std::string invokeHold = "[" + Report.Invoker + "]: ";

    std::string DebugMessage = timeHold + invokeHold + Report.Message;
    std::cout << DebugMessage << std::endl;
}