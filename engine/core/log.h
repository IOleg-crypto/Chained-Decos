#ifndef CH_LOG_H
#define CH_LOG_H

#include "raylib.h"
#include <format>
#include <string>

namespace CHEngine
{
enum class LogLevel
{
    LogTrace = LOG_TRACE,
    LogInfo = LOG_INFO,
    LogWarning = LOG_WARNING,
    LogError = LOG_ERROR,
    LogFatal = LOG_FATAL,
    LogNone = LOG_NONE
};

class Log
{
public:
    static void SetLogLevel(LogLevel level)
    {
        s_LogLevel = level;
        ::SetTraceLogLevel((int)level);
    }
    static LogLevel GetLogLevel()
    {
        return s_LogLevel;
    }

    // Core logging functions
    template <typename... Args> static void CoreTrace(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_TRACE, "[CORE] %s", message.c_str());
    }

    template <typename... Args> static void CoreInfo(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_INFO, "[CORE] %s", message.c_str());
    }

    template <typename... Args> static void CoreWarn(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_WARNING, "[CORE] %s", message.c_str());
    }

    template <typename... Args> static void CoreError(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_ERROR, "[CORE] %s", message.c_str());
    }

    template <typename... Args> static void CoreFatal(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_FATAL, "[CORE] %s", message.c_str());
    }

    // Client logging functions
    template <typename... Args> static void ClientTrace(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_TRACE, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args> static void ClientInfo(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_INFO, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args> static void ClientWarn(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_WARNING, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args> static void ClientError(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_ERROR, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args> static void ClientFatal(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_FATAL, "[CLIENT] %s", message.c_str());
    }

private:
#ifdef CH_DEBUG
    inline static LogLevel s_LogLevel = LogLevel::LogTrace;
#else
    inline static LogLevel s_LogLevel = LogLevel::LogInfo;
#endif
};
} // namespace CHEngine

// Core logging macros
#define CH_CORE_TRACE(...)                                                                                             \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogTrace)                                              \
    ::CHEngine::Log::CoreTrace(__VA_ARGS__)
#define CH_CORE_INFO(...)                                                                                              \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogInfo)                                               \
    ::CHEngine::Log::CoreInfo(__VA_ARGS__)
#define CH_CORE_WARN(...)                                                                                              \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogWarning)                                            \
    ::CHEngine::Log::CoreWarn(__VA_ARGS__)
#define CH_CORE_ERROR(...)                                                                                             \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogError)                                              \
    ::CHEngine::Log::CoreError(__VA_ARGS__)
#define CH_CORE_FATAL(...)                                                                                             \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogFatal)                                              \
    ::CHEngine::Log::CoreFatal(__VA_ARGS__)

// Client logging macros
#define CH_TRACE(...)                                                                                                  \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogTrace)                                              \
    ::CHEngine::Log::ClientTrace(__VA_ARGS__)
#define CH_INFO(...)                                                                                                   \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogInfo)                                               \
    ::CHEngine::Log::ClientInfo(__VA_ARGS__)
#define CH_WARN(...)                                                                                                   \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogWarning)                                            \
    ::CHEngine::Log::ClientWarn(__VA_ARGS__)
#define CH_ERROR(...)                                                                                                  \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogError)                                              \
    ::CHEngine::Log::ClientError(__VA_ARGS__)
#define CH_FATAL(...)                                                                                                  \
    if (::CHEngine::Log::GetLogLevel() <= ::CHEngine::LogLevel::LogFatal)                                              \
    ::CHEngine::Log::ClientFatal(__VA_ARGS__)

#endif // CH_LOG_H
