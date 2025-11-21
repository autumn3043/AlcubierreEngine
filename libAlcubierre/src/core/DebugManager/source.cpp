#include "core/DebugManager/public.h"
#include "core/DebugManager/private.h"

#include <iostream>
#include <utility>
#include <string>
#include <sstream>
#include <iomanip>

const int PRINT_MIN_VERBOSITY = 0;
const int WRITE_MIN_VERBOSITY = 0;

DebugManager& DebugManager::GetDebugManager() {
    static DebugManager Instance;
    return Instance;
}

DebugManager::DebugManager() {
    if(!PrivatePtr) PrivatePtr = new DebugManagerImpl();
}

DebugManager::~DebugManager() {
    Log("Note static entity deconstruction.");
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

        //To access DM globally it has to be static. This makes logfile static. If something tries to log during static deconstruction DM will exist but logfile will not - > segfault. 
        //Below workaround avoids a segfault by creating new logfile object. This is to be avoided. Handled deconstruct should finish BEFORE any static deconstruct.
        if(!logfile.is_open()) {
            std::ofstream logfileTROUBLE;
            logfileTROUBLE.open("program.log", std::ios::app);
            logfileTROUBLE << "::TROUBLE:: " << DebugMessage << std::endl;
            logfileTROUBLE.close();
        } else {
            logfile << DebugMessage << std::endl;
        }
    }

    if(Report->Level >= PRINT_MIN_VERBOSITY) std::cout << DebugMessage << std::endl;
}

DebugManager::Punchcard::Punchcard() {
    punch();
}

#include <chrono>

void DebugManager::Punchcard::punch() {
    entry = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int DebugManager::Punchcard::delta() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - entry;
}