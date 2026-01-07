#ifndef CD_CORE_LOG_H
#define CD_CORE_LOG_H

#include <raylib.h>
#include <string>

namespace CHEngine
{

// Forward declare Log class to use in macros
class Log;

// Internal helper to convert std::string to const char* for variadic functions
template <typename T> static inline T ConvertArg(T arg)
{
    return arg;
}
static inline const char *ConvertArg(const std::string &arg)
{
    return arg.c_str();
}

// Core logging (for engine internals)
#define CD_CORE_TRACE(...) ::CHEngine::Log::CoreTrace(__VA_ARGS__)
#define CD_CORE_INFO(...) ::CHEngine::Log::CoreInfo(__VA_ARGS__)
#define CD_CORE_WARN(...) ::CHEngine::Log::CoreWarn(__VA_ARGS__)
#define CD_CORE_ERROR(...) ::CHEngine::Log::CoreError(__VA_ARGS__)
#define CD_CORE_FATAL(...) ::CHEngine::Log::CoreFatal(__VA_ARGS__)

// Client logging (for game/editor code)
#define CD_TRACE(...) ::CHEngine::Log::ClientTrace(__VA_ARGS__)
#define CD_INFO(...) ::CHEngine::Log::ClientInfo(__VA_ARGS__)
#define CD_WARN(...) ::CHEngine::Log::ClientWarn(__VA_ARGS__)
#define CD_ERROR(...) ::CHEngine::Log::ClientError(__VA_ARGS__)
#define CD_FATAL(...) ::CHEngine::Log::ClientFatal(__VA_ARGS__)

class Log
{
public:
    static void Init()
    {
    } // Dummy for compatibility with Hazel-style entry point

    // Core logging functions
    template <typename... Args> static void CoreTrace(const char *format, Args... args)
    {
        TraceLog(LOG_TRACE, ("[CORE] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void CoreInfo(const char *format, Args... args)
    {
        TraceLog(LOG_INFO, ("[CORE] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void CoreWarn(const char *format, Args... args)
    {
        TraceLog(LOG_WARNING, ("[CORE] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void CoreError(const char *format, Args... args)
    {
        TraceLog(LOG_ERROR, ("[CORE] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void CoreFatal(const char *format, Args... args)
    {
        TraceLog(LOG_FATAL, ("[CORE] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    // Client logging functions
    template <typename... Args> static void ClientTrace(const char *format, Args... args)
    {
        TraceLog(LOG_TRACE, ("[CLIENT] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void ClientInfo(const char *format, Args... args)
    {
        TraceLog(LOG_INFO, ("[CLIENT] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void ClientWarn(const char *format, Args... args)
    {
        TraceLog(LOG_WARNING, ("[CLIENT] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void ClientError(const char *format, Args... args)
    {
        TraceLog(LOG_ERROR, ("[CLIENT] " + std::string(format)).c_str(), ConvertArg(args)...);
    }

    template <typename... Args> static void ClientFatal(const char *format, Args... args)
    {
        TraceLog(LOG_FATAL, ("[CLIENT] " + std::string(format)).c_str(), ConvertArg(args)...);
    }
};

} // namespace CHEngine

#endif // CD_CORE_LOG_H
