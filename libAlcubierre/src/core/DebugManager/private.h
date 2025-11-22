#ifndef ALCENGINE_CORE_DEBUGMANAGER_PRIVATE_H
#define ALCENGINE_CORE_DEBUGMANAGER_PRIVATE_H

#include <fstream>

class DebugReport;

class DebugManagerImpl {
    public:
        DebugManagerImpl();
        ~DebugManagerImpl();

        void InternalLog(DebugReport* Report, bool Write = true);

        int logToLogfileMinVerb = 0;
        int logToConsoleMinVerb = 1;

    private:
        std::ofstream logfile;
};

#endif 