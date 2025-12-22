#pragma once

#include <raylib.h>
#include <string>

namespace ChainedEngine
{

// Forward declare Log class to use in macros
class Log;

// Core logging (for engine internals)
#define CD_CORE_TRACE(...) ::ChainedEngine::Log::CoreTrace(__VA_ARGS__)
#define CD_CORE_INFO(...) ::ChainedEngine::Log::CoreInfo(__VA_ARGS__)
#define CD_CORE_WARN(...) ::ChainedEngine::Log::CoreWarn(__VA_ARGS__)
#define CD_CORE_ERROR(...) ::ChainedEngine::Log::CoreError(__VA_ARGS__)
#define CD_CORE_FATAL(...) ::ChainedEngine::Log::CoreFatal(__VA_ARGS__)

// Client logging (for game/editor code)
#define CD_TRACE(...) ::ChainedEngine::Log::ClientTrace(__VA_ARGS__)
#define CD_INFO(...) ::ChainedEngine::Log::ClientInfo(__VA_ARGS__)
#define CD_WARN(...) ::ChainedEngine::Log::ClientWarn(__VA_ARGS__)
#define CD_ERROR(...) ::ChainedEngine::Log::ClientError(__VA_ARGS__)
#define CD_FATAL(...) ::ChainedEngine::Log::ClientFatal(__VA_ARGS__)

class Log
{
public:
    // Core logging functions
    template <typename... Args> static void CoreTrace(const char *format, Args... args)
    {
        TraceLog(LOG_TRACE, ("[CORE] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void CoreInfo(const char *format, Args... args)
    {
        TraceLog(LOG_INFO, ("[CORE] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void CoreWarn(const char *format, Args... args)
    {
        TraceLog(LOG_WARNING, ("[CORE] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void CoreError(const char *format, Args... args)
    {
        TraceLog(LOG_ERROR, ("[CORE] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void CoreFatal(const char *format, Args... args)
    {
        TraceLog(LOG_FATAL, ("[CORE] " + std::string(format)).c_str(), args...);
    }

    // Client logging functions
    template <typename... Args> static void ClientTrace(const char *format, Args... args)
    {
        TraceLog(LOG_TRACE, ("[APP] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void ClientInfo(const char *format, Args... args)
    {
        TraceLog(LOG_INFO, ("[APP] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void ClientWarn(const char *format, Args... args)
    {
        TraceLog(LOG_WARNING, ("[APP] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void ClientError(const char *format, Args... args)
    {
        TraceLog(LOG_ERROR, ("[APP] " + std::string(format)).c_str(), args...);
    }

    template <typename... Args> static void ClientFatal(const char *format, Args... args)
    {
        TraceLog(LOG_FATAL, ("[APP] " + std::string(format)).c_str(), args...);
    }
};

} // namespace ChainedEngine
