#ifndef DEBUG_MANAGER_ALE_H
#define DEBUG_MANAGER_ALE_H 

#include <exception>
#include <ctime>
#include <stacktrace>
#include <optional>

class DebugManager {
    public:
        class DebugReport {
            public:
                DebugReport(
                    std::string msg_ = "",
                    std::string invoker_ = "",
                    std::stacktrace trace_ = {},
                    std::time_t time_ = std::time(nullptr), 
                    int level_ = 0
                ) :
                    Message(msg_),
                    Trace(trace_),
                    Invoker(invoker_),
                    Time(time_),
                    Level(level_)
                {};

                const std::string Message;
                const std::stacktrace Trace;
                const std::string Invoker;
                const std::time_t Time;
                const int Level;
        };

        class AlcExcept : public std::exception {
            public:
                AlcExcept(DebugReport report_) : Report(report_) {}

                DebugReport Get() {return Report;} 
            
            private:
                DebugReport Report;
        };

        static DebugManager& GetDebugManager();

        static void Log(std::string, bool Write = true);
        static void Log(std::exception, bool Write = true);
        static void Log(DebugReport, bool Write = true);
        static void Log(AlcExcept, bool Write = true);

    private:
        DebugManager() {};

        void InternalLog(DebugReport Report, bool Write = true);
        
        void WriteToLogFile(DebugReport);
};

#endif