#ifndef CH_LOG_H
#define CH_LOG_H

#include <raylib.h>
#include <string>

namespace CH
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
} // namespace CH

// Core logging macros
#define CH_CORE_TRACE(...) ::CH::Log::CoreTrace(__VA_ARGS__)
#define CH_CORE_INFO(...) ::CH::Log::CoreInfo(__VA_ARGS__)
#define CH_CORE_WARN(...) ::CH::Log::CoreWarn(__VA_ARGS__)
#define CH_CORE_ERROR(...) ::CH::Log::CoreError(__VA_ARGS__)
#define CH_CORE_FATAL(...) ::CH::Log::CoreFatal(__VA_ARGS__)

// Client logging macros
#define CH_TRACE(...) ::CH::Log::ClientTrace(__VA_ARGS__)
#define CH_INFO(...) ::CH::Log::ClientInfo(__VA_ARGS__)
#define CH_WARN(...) ::CH::Log::ClientWarn(__VA_ARGS__)
#define CH_ERROR(...) ::CH::Log::ClientError(__VA_ARGS__)
#define CH_FATAL(...) ::CH::Log::ClientFatal(__VA_ARGS__)

#endif // CH_LOG_H
