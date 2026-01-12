#ifndef CH_LOG_H
#define CH_LOG_H

#include <raylib.h>
#include <string>

namespace CHEngine
{
// Internal helper to convert std::string to const char* for variadic functions
template <typename T> static inline T ConvertLogArg(T arg)
{
    return arg;
}
static inline const char *ConvertLogArg(const std::string &arg)
{
    return arg.c_str();
}

class Log
{
public:
    // Core logging functions
    template <typename... Args> static void CoreTrace(const char *format, Args... args)
    {
        TraceLog(LOG_TRACE, ("[CORE] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void CoreInfo(const char *format, Args... args)
    {
        TraceLog(LOG_INFO, ("[CORE] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void CoreWarn(const char *format, Args... args)
    {
        TraceLog(LOG_WARNING, ("[CORE] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void CoreError(const char *format, Args... args)
    {
        TraceLog(LOG_ERROR, ("[CORE] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void CoreFatal(const char *format, Args... args)
    {
        TraceLog(LOG_FATAL, ("[CORE] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    // Client logging functions
    template <typename... Args> static void ClientTrace(const char *format, Args... args)
    {
        TraceLog(LOG_TRACE, ("[CLIENT] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void ClientInfo(const char *format, Args... args)
    {
        TraceLog(LOG_INFO, ("[CLIENT] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void ClientWarn(const char *format, Args... args)
    {
        TraceLog(LOG_WARNING, ("[CLIENT] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void ClientError(const char *format, Args... args)
    {
        TraceLog(LOG_ERROR, ("[CLIENT] " + std::string(format)).c_str(), ConvertLogArg(args)...);
    }

    template <typename... Args> static void ClientFatal(const char *format, Args... args)
    {
        TraceLog(LOG_FATAL, ("[CLIENT] " + std::string(format)).c_str(), ConvertLogArg(args)...);
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
