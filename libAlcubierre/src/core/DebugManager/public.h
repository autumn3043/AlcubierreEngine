#ifndef ALCENGINE_CORE_DEBUGMANAGER_PUBLIC_H
#define ALCENGINE_CORE_DEBUGMANAGER_PUBLIC_H

#include <exception>
#include <ctime>
#include <string>
#include <cstdint>

class DebugReport {
    public:
        DebugReport(
            std::string msg_,
            int level_ = 0,
            std::string invoker_ = "",
            std::time_t time_ = std::time(nullptr) 
        ) :
            Message(msg_),
            Level(level_),
            Invoker(invoker_),
            Time(time_)
        {};

        const std::string Message;
        const int Level;
        const std::string Invoker;
        const std::time_t Time;
};

class AlcEngineException : public std::exception {
    public:
        AlcEngineException(DebugReport report_) : Report(std::move(report_)) {}

        DebugReport Get() const {return Report;}
        const char* what() const noexcept override { return Report.Message.c_str(); }

    private:
        DebugReport Report;
};

class DebugManagerImpl;

class DebugManager{
    public:
        static DebugManager& GetDebugManager();

        void Log();
        void Log(std::string, int Level = 0, bool Write = true);

        template <typename T>
        std::enable_if_t<std::is_arithmetic_v<T>, void>
        Log(T value, int Level = 0, bool Write = true) {
            Log(std::to_string(value), Level, Write);
        }

        void Log(std::exception, bool Write = true);
        void Log(DebugReport, bool Write = true);
        void Log(AlcEngineException, bool Write = true);

        class Punchcard {
            public:
                void punch();
                uint64_t delta(int digits = 3);

                Punchcard();
            private:
                uint64_t entry;
        };

        int SetLogToLogfileMinVerbosity(int minVerbosity);
        int SetLogToConsoleMinVerbosity(int minVerbosity);

    private:
        DebugManager();
        ~DebugManager();

        DebugManagerImpl* PrivatePtr = nullptr;
};

inline DebugManager& DM() { return DebugManager::GetDebugManager(); }

#endif