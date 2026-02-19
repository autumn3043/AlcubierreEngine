#ifndef ALCENGINE_CORE_DEBUGMANAGER_PRIVATE_H
#define ALCENGINE_CORE_DEBUGMANAGER_PRIVATE_H

#include <fstream>
#include <thread>

class DebugReport;

class DebugManagerImpl {
    public:
        DebugManagerImpl();
        ~DebugManagerImpl();

        std::mutex loggingThreadLock;
        void InternalLog(DebugReport* Report, bool Write = true);

        int logToLogfileMinVerb = 0;
        int logToConsoleMinVerb = 1;

    private:
        std::ofstream logfile;
};

#endif 