#include "core/DebugManager/public.h"
#include "core/DebugManager/private.h"

#include <iostream>
#include <utility>
#include <string>
#include <sstream>
#include <iomanip>

DebugManager& DebugManager::GetDebugManager() {
    static DebugManager Instance;
    return Instance;
}

DebugManager::DebugManager() {
    if(!PrivatePtr) PrivatePtr = new DebugManagerImpl();
}

DebugManager::~DebugManager() {
    if(PrivatePtr) delete PrivatePtr;
}

// Public methods for shorthand interfacing
    void DebugManager::Log(std::string Message, int Level, bool Write) {
        DebugReport* Report = new DebugReport(Message, Level);
        PrivatePtr->InternalLog(Report, Write);
        delete Report;
    }

    void DebugManager::Log(std::exception exception, bool Write) {
        DebugReport* Report = new DebugReport(std::string(exception.what()));
        PrivatePtr->InternalLog(Report, Write);
        delete Report;
    } 

    void DebugManager::Log(AlcEngineException exception, bool Write) {
        DebugReport* Report = new DebugReport(exception.Get());
        PrivatePtr->InternalLog(Report, Write);
        delete Report;
    }

    void DebugManager::Log(DebugReport Report, bool Write) {
    PrivatePtr->InternalLog(&Report, Write);
    }

DebugManagerImpl::DebugManagerImpl() {
    logfile.open("program.log", std::ios::trunc);
    logfile << "<--DebugManager Instance Created-->" << std::endl;
}

DebugManagerImpl::~DebugManagerImpl() {
    logfile << std::endl;
    logfile.close();
}

void DebugManagerImpl::InternalLog(DebugReport* Report, bool Write) {
    std::ostringstream oss;

    if(Report->Time) {
        tm _time = *localtime(&Report->Time);
        oss << "@["
            << std::setw(2) << std::setfill('0') << _time.tm_hour << ":"
            << std::setw(2) << std::setfill('0') << _time.tm_min << ":"
            << std::setw(2) << std::setfill('0') << _time.tm_sec << "]: ";
    }

    if(Report->Invoker != "") {
        oss << "["
            << Report->Invoker
            << "]: ";
    }

    oss << Report->Message;

    std::string DebugMessage = oss.str();

    if(Report->Level >= WRITE_MIN_VERBOSITY) {
        //TEMPORARY BODGE FIX THE DI STUFF U DUMBY DUMB DUMB
        if(!logfile.is_open()) {
            logfile.open("program.log", std::ios::app);
            logfile << DebugMessage << std::endl;
            logfile.close();
        } else {
            logfile << DebugMessage << std::endl;
        }
    }

    if(Report->Level >= PRINT_MIN_VERBOSITY) std::cout << DebugMessage << std::endl;
}