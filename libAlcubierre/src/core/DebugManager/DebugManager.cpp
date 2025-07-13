#include "core/DebugManager/DebugManager.h"

#include <iostream>
#include <utility>
#include <string>
#include <sstream>
#include <iomanip>

DebugManager& DebugManager::GetDebugManager() {
    static DebugManager instance;

    return instance;
}

//Public statics for shorthand interfacing

void DebugManager::Log(std::string Message, bool Write) { 
    AlcExceptions::DebugReport Report(Message);
    GetDebugManager().InternalLog(std::move(Report), Write);
}

void DebugManager::Log(std::exception exception, bool Write) {
    AlcExceptions::DebugReport Report(std::string(exception.what()));
    GetDebugManager().InternalLog(std::move(Report), Write);
} 

void DebugManager::Log(AlcExceptions::AlcExcept exception, bool Write) {
    GetDebugManager().InternalLog(std::move(exception.Get()), Write);
}

void DebugManager::Log(AlcExceptions::DebugReport Report, bool Write) {
    GetDebugManager().InternalLog(std::move(Report), Write);
}

//Private management of demystified input

void DebugManager::InternalLog(AlcExceptions::DebugReport Report, bool Write) {

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