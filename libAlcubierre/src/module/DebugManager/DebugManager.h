#ifndef DEBUG_MANAGER_ALE_H
#define DEBUG_MANAGER_ALE_H 

#include <exception>
#include <ctime>
#include <stacktrace>

namespace AlcExceptions {
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
            explicit AlcExcept(DebugReport report_) : Report(std::move(report_)) {}

            DebugReport Get() const {return Report;}
            const char* what() const noexcept override {
                return Report.Message.c_str();
            }

        private:
            DebugReport Report;
    };

    class UnknownExcept : public AlcExcept {
        public:
            UnknownExcept() : AlcExcept(DebugReport("Unknown exception")) {}
    };

    class RuntimeExcept : public AlcExcept {
        public:
            RuntimeExcept() : AlcExcept(DebugReport("Runtime exception")) {}
    };

    class SetupFail : public AlcExcept {
        public:
            SetupFail() : AlcExcept(DebugReport("Setup failure")) {}
    };

    class ConfigAssignFail : public AlcExcept {
        public:
            ConfigAssignFail() : AlcExcept(DebugReport("Config assignment failure")) {}
    };

    class IOError : public AlcExcept {
        public:
            IOError() : AlcExcept(DebugReport("Error reading filesystem")) {}
    };
}

class DebugManager {
    public:
        static DebugManager& GetDebugManager();

        static void Log(std::string, bool Write = true);
        static void Log(std::exception, bool Write = true);
        static void Log(AlcExceptions::DebugReport, bool Write = true);
        static void Log(AlcExceptions::AlcExcept, bool Write = true);

    private:
        DebugManager() {};

        void InternalLog(AlcExceptions::DebugReport Report, bool Write = true);
        
        void WriteToLogFile(std::string Line);
};

#endif