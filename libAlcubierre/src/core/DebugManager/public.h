#ifndef ALCENGINE_MODULE_DEBUGMANAGER_PUBLIC_H
#define ALCENGINE_MODULE_DEBUGMANAGER_PUBLIC_H

#include <exception>
#include <ctime>
#include <stacktrace>

class DebugManagerImpl;

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
            Invoker(invoker_),
            Trace(trace_),
            Time(time_),
            Level(level_)
        {};

        const std::string Message;
        const std::string Invoker;
        const std::stacktrace Trace;
        const std::time_t Time;
        const int Level;
};

class AlcEngineException : public std::exception {
    public:
        AlcExcept(DebugReport report_) : Report(std::move(report_)) {}

        DebugReport Get() const {return Report;}
        const char* what() const noexcept override {
            return Report.Message.c_str();
            }

    private:
        DebugReport Report;
};

class DebugManager {
    public:
        static DebugManager& Get();

        static void Log(std::string, bool Write = true);
        static void Log(std::exception, bool Write = true);
        static void Log(DebugReport, bool Write = true);
        static void Log(AlcEngineException, bool Write = true);

    private:
        DebugManager();
        ~DebugManager();

        void InternalLog(DebugReport Report, bool Write = true);

        DebugManagerImpl* PrivatePtr;

};

#endif