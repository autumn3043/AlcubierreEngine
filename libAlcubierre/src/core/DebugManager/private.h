#ifndef ALCENGINE_CORE_DEBUGMANAGER_PRIVATE_H
#define ALCENGINE_CORE_DEBUGMANAGER_PRIVATE_H

#include <fstream>

class DebugReport;

class DebugManagerImpl {
    public:
        DebugManagerImpl();
        ~DebugManagerImpl();

        void InternalLog(DebugReport* Report, bool Write = true);

    private:
        std::ofstream logfile;
};

#endif 