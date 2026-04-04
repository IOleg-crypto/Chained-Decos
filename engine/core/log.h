#ifndef CH_LOG_H
#define CH_LOG_H

#include <format>
#include <string>
#include <iostream>

namespace CHEngine
{
enum class LogLevel
{
    LogTrace = 0,
    LogInfo,
    LogWarning,
    LogError,
    LogFatal,
    LogNone
};

    using LogCallbackFn = void(*)(const char*, int);

    class Log
    {
    public:
        static void SetLogLevel(LogLevel level)
        {
            s_LogLevel = level;
        }
        static LogLevel GetLogLevel()
        {
            return s_LogLevel;
        }

        static void SetLogCallback(LogCallbackFn callback)
        {
            s_LogCallback = callback;
        }

        // Core logging functions
        template <typename... Args> static void CoreTrace(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogTrace) LogMessage("[CORE] [TRACE] ", LogLevel::LogTrace, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void CoreInfo(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogInfo) LogMessage("[CORE] [INFO]  ", LogLevel::LogInfo, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void CoreWarn(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogWarning) LogMessage("[CORE] [WARN]  ", LogLevel::LogWarning, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void CoreError(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogError) LogMessage("[CORE] [ERROR] ", LogLevel::LogError, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void CoreFatal(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogFatal) LogMessage("[CORE] [FATAL] ", LogLevel::LogFatal, fmt, std::forward<Args>(args)...);
        }

        // Client logging functions
        template <typename... Args> static void ClientTrace(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogTrace) LogMessage("[CLIENT] [TRACE] ", LogLevel::LogTrace, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void ClientInfo(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogInfo) LogMessage("[CLIENT] [INFO]  ", LogLevel::LogInfo, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void ClientWarn(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogWarning) LogMessage("[CLIENT] [WARN]  ", LogLevel::LogWarning, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void ClientError(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogError) LogMessage("[CLIENT] [ERROR] ", LogLevel::LogError, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args> static void ClientFatal(std::format_string<Args...> fmt, Args&&... args)
        {
            if (s_LogLevel <= LogLevel::LogFatal) LogMessage("[CLIENT] [FATAL] ", LogLevel::LogFatal, fmt, std::forward<Args>(args)...);
        }

    private:
        template <typename... Args>
        static void LogMessage(const char* prefix, LogLevel level, std::format_string<Args...> fmt, Args&&... args)
        {
            std::string message = std::format(fmt, std::forward<Args>(args)...);
            std::string fullMessage = std::string(prefix) + message;
            
            // Output to system console
            std::cout << fullMessage << std::endl;
            
            // Runtime callback for Editor/UI integration
            if (s_LogCallback)
            {
                s_LogCallback(fullMessage.c_str(), (int)level);
            }
        }

    private:
#ifdef CH_DEBUG
        inline static LogLevel s_LogLevel = LogLevel::LogTrace;
#else
        inline static LogLevel s_LogLevel = LogLevel::LogInfo;
#endif
        inline static LogCallbackFn s_LogCallback = nullptr;
    };
} // namespace CHEngine

// Core logging macros
#define CH_CORE_TRACE(...)  ::CHEngine::Log::CoreTrace(__VA_ARGS__)
#define CH_CORE_INFO(...)   ::CHEngine::Log::CoreInfo(__VA_ARGS__)
#define CH_CORE_WARN(...)   ::CHEngine::Log::CoreWarn(__VA_ARGS__)
#define CH_CORE_ERROR(...)  ::CHEngine::Log::CoreError(__VA_ARGS__)
#define CH_CORE_FATAL(...)  ::CHEngine::Log::CoreFatal(__VA_ARGS__)

#define CH_CORE_TRACE_ONCE(...)                                                                                        \
    { static bool warned = false; if (!warned) { CH_CORE_TRACE(__VA_ARGS__); warned = true; } }
#define CH_CORE_INFO_ONCE(...)                                                                                         \
    { static bool warned = false; if (!warned) { CH_CORE_INFO(__VA_ARGS__); warned = true; } }
#define CH_CORE_WARN_ONCE(...)                                                                                         \
    { static bool warned = false; if (!warned) { CH_CORE_WARN(__VA_ARGS__); warned = true; } }
#define CH_CORE_ERROR_ONCE(...)                                                                                        \
    { static bool warned = false; if (!warned) { CH_CORE_ERROR(__VA_ARGS__); warned = true; } }

// Client logging macros
#define CH_TRACE(...)  ::CHEngine::Log::ClientTrace(__VA_ARGS__)
#define CH_INFO(...)   ::CHEngine::Log::ClientInfo(__VA_ARGS__)
#define CH_WARN(...)   ::CHEngine::Log::ClientWarn(__VA_ARGS__)
#define CH_ERROR(...)  ::CHEngine::Log::ClientError(__VA_ARGS__)
#define CH_FATAL(...)  ::CHEngine::Log::ClientFatal(__VA_ARGS__)

#define CH_TRACE_ONCE(...)                                                                                             \
    { static bool warned = false; if (!warned) { CH_TRACE(__VA_ARGS__); warned = true; } }
#define CH_INFO_ONCE(...)                                                                                              \
    { static bool warned = false; if (!warned) { CH_INFO(__VA_ARGS__); warned = true; } }
#define CH_WARN_ONCE(...)                                                                                              \
    { static bool warned = false; if (!warned) { CH_WARN(__VA_ARGS__); warned = true; } }
#define CH_ERROR_ONCE(...)                                                                                             \
    { static bool warned = false; if (!warned) { CH_ERROR(__VA_ARGS__); warned = true; } }

#endif // CH_LOG_H
