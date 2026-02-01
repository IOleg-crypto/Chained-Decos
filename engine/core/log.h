#ifndef CH_LOG_H
#define CH_LOG_H

#include <format>
#include "raylib.h"
#include <string>

namespace CHEngine
{
class Log
{
public:
    // Core logging functions
    template <typename... Args>
    static void CoreTrace(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_TRACE, "[CORE] %s", message.c_str());
    }

    template <typename... Args>
    static void CoreInfo(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_INFO, "[CORE] %s", message.c_str());
    }

    template <typename... Args>
    static void CoreWarn(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_WARNING, "[CORE] %s", message.c_str());
    }

    template <typename... Args>
    static void CoreError(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_ERROR, "[CORE] %s", message.c_str());
    }

    template <typename... Args>
    static void CoreFatal(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_FATAL, "[CORE] %s", message.c_str());
    }

    // Client logging functions
    template <typename... Args>
    static void ClientTrace(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_TRACE, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args>
    static void ClientInfo(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_INFO, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args>
    static void ClientWarn(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_WARNING, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args>
    static void ClientError(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_ERROR, "[CLIENT] %s", message.c_str());
    }

    template <typename... Args>
    static void ClientFatal(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        TraceLog(LOG_FATAL, "[CLIENT] %s", message.c_str());
    }
};
} // namespace CHEngine

// Core logging macros
#define CH_CORE_TRACE(...) ::CHEngine::Log::CoreTrace(__VA_ARGS__)
#define CH_CORE_INFO(...) ::CHEngine::Log::CoreInfo(__VA_ARGS__)
#define CH_CORE_WARN(...) ::CHEngine::Log::CoreWarn(__VA_ARGS__)
#define CH_CORE_ERROR(...) ::CHEngine::Log::CoreError(__VA_ARGS__)
#define CH_CORE_FATAL(...) ::CHEngine::Log::CoreFatal(__VA_ARGS__)

// Client logging macros
#define CH_TRACE(...) ::CHEngine::Log::ClientTrace(__VA_ARGS__)
#define CH_INFO(...) ::CHEngine::Log::ClientInfo(__VA_ARGS__)
#define CH_WARN(...) ::CHEngine::Log::ClientWarn(__VA_ARGS__)
#define CH_ERROR(...) ::CHEngine::Log::ClientError(__VA_ARGS__)
#define CH_FATAL(...) ::CHEngine::Log::ClientFatal(__VA_ARGS__)

#endif // CH_LOG_H
