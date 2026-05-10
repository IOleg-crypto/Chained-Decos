#ifndef CH_LOG_H
#define CH_LOG_H

#include <chrono>
#include <ctime>
#include <deque>
#include <format>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

struct BufferedLogEntry
{
    LogLevel level;
    std::string message;
    std::string timestamp;
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

        static std::vector<BufferedLogEntry> ConsumeBufferedMessages()
        {
            std::lock_guard<std::mutex> lock(s_BufferMutex);

            std::vector<BufferedLogEntry> messages;
            messages.reserve(s_Buffer.size());

            while (!s_Buffer.empty())
            {
                messages.push_back(std::move(s_Buffer.front()));
                s_Buffer.pop_front();
            }

            return messages;
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
            std::string timestamp = GetCurrentTimestamp();
            std::string message = std::format(fmt, std::forward<Args>(args)...);
            std::string fullMessage = std::string(prefix) + message;
            // Output to system console
            {
                std::lock_guard<std::mutex> lock(s_ConsoleMutex);
                std::cout << fullMessage << std::endl;
            }

            {
                std::lock_guard<std::mutex> lock(s_BufferMutex);
                s_Buffer.push_back({level, fullMessage, timestamp});
                if (s_Buffer.size() > MAX_BUFFERED_MESSAGES)
                {
                    s_Buffer.pop_front();
                }
            }
            
            // Runtime callback for Editor/UI integration
            if (s_LogCallback)
            {
                std::lock_guard<std::mutex> lock(s_ConsoleMutex); // Reuse console mutex for callback safety
                s_LogCallback(fullMessage.c_str(), (int)level);
            }
        }

        static std::string GetCurrentTimestamp()
        {
            using namespace std::chrono;

            auto now = system_clock::now();
            auto in_time_t = system_clock::to_time_t(now);

            std::tm time_info;
#if defined(_MSC_VER)
            localtime_s(&time_info, &in_time_t);
#elif defined(_WIN32)
            std::tm* tm_ptr = std::localtime(&in_time_t);
            if (tm_ptr)
            {
                time_info = *tm_ptr;
            }
#else
            localtime_r(&in_time_t, &time_info);
#endif

            std::stringstream ss;
            ss << "[" << std::put_time(&time_info, "%H:%M:%S") << "] ";
            return ss.str();
        }

    private:
#ifdef CH_DEBUG
        inline static LogLevel s_LogLevel = LogLevel::LogTrace;
#else
        inline static LogLevel s_LogLevel = LogLevel::LogInfo;
#endif
        inline static LogCallbackFn s_LogCallback = nullptr;
        inline static std::deque<BufferedLogEntry> s_Buffer;
        inline static std::mutex s_BufferMutex;
        inline static std::mutex s_ConsoleMutex;
        static constexpr size_t MAX_BUFFERED_MESSAGES = 10000;
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
