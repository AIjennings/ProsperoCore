#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

namespace prospero {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Syscall,
    Gpu
};

class Logger {
public:
    static Logger& instance() {
        static Logger s_instance;
        return s_instance;
    }

    void init() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
    }

    void log(LogLevel level, const char* category, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const char* color = "\033[0m";
        const char* tag = "INFO";

        switch (level) {
            case LogLevel::Trace:   color = "\033[90m"; tag = "TRACE"; break;
            case LogLevel::Debug:   color = "\033[36m"; tag = "DEBUG"; break;
            case LogLevel::Info:    color = "\033[32m"; tag = "INFO "; break;
            case LogLevel::Warn:    color = "\033[33m"; tag = "WARN "; break;
            case LogLevel::Error:   color = "\033[31;1m"; tag = "ERROR"; break;
            case LogLevel::Syscall: color = "\033[35m"; tag = "SYSC "; break;
            case LogLevel::Gpu:     color = "\033[94m"; tag = "GPU  "; break;
        }

        std::cout << color << "[" << tag << "] [" << category << "] " << message << "\033[0m\n" << std::flush;
    }

private:
    Logger() { init(); }
    std::mutex m_mutex;
};

#define PROSPERO_LOG_INFO(cat, msg) do { \
    std::ostringstream ss; ss << msg; \
    ::prospero::Logger::instance().log(::prospero::LogLevel::Info, cat, ss.str()); \
} while(0)

#define PROSPERO_LOG_WARN(cat, msg) do { \
    std::ostringstream ss; ss << msg; \
    ::prospero::Logger::instance().log(::prospero::LogLevel::Warn, cat, ss.str()); \
} while(0)

#define PROSPERO_LOG_ERROR(cat, msg) do { \
    std::ostringstream ss; ss << msg; \
    ::prospero::Logger::instance().log(::prospero::LogLevel::Error, cat, ss.str()); \
} while(0)

#define PROSPERO_LOG_DEBUG(cat, msg) do { \
    std::ostringstream ss; ss << msg; \
    ::prospero::Logger::instance().log(::prospero::LogLevel::Debug, cat, ss.str()); \
} while(0)

#define PROSPERO_LOG_SYSCALL(cat, msg) do { \
    std::ostringstream ss; ss << msg; \
    ::prospero::Logger::instance().log(::prospero::LogLevel::Syscall, cat, ss.str()); \
} while(0)

#define PROSPERO_LOG_GPU(cat, msg) do { \
    std::ostringstream ss; ss << msg; \
    ::prospero::Logger::instance().log(::prospero::LogLevel::Gpu, cat, ss.str()); \
} while(0)

} // namespace prospero
