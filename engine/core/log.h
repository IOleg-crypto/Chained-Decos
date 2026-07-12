#ifndef CH_LOG_H
#define CH_LOG_H

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace Chained
{
enum class LogLevel
{
    LogTrace = spdlog::level::trace,
    LogInfo = spdlog::level::info,
    LogWarning = spdlog::level::warn,
    LogError = spdlog::level::err,
    LogFatal = spdlog::level::critical,
    LogNone = spdlog::level::off
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
    static void Init()
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%H:%M:%S] %v%$");

        auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>([&](const spdlog::details::log_msg& msg) {
            std::string message(msg.payload.data(), msg.payload.size());
            std::string timestamp = std::format("[{:02d}:{:02d}:{:02d}] ", 
                msg.time.time_since_epoch().count() / 3600000000 % 24, // Simplified timestamp for buffer
                msg.time.time_since_epoch().count() / 60000000 % 60,
                msg.time.time_since_epoch().count() / 1000000 % 60);

            std::lock_guard<std::mutex> lock(s_BufferMutex);
            s_Buffer.push_back({ static_cast<LogLevel>(msg.level), message, timestamp });
            if (s_Buffer.size() > MAX_BUFFERED_MESSAGES)
                s_Buffer.pop_front();

            if (s_LogCallback)
                s_LogCallback(message.c_str(), static_cast<int>(msg.level));
        });

        std::vector<spdlog::sink_ptr> sinks { console_sink, callback_sink };
        
        s_CoreLogger = std::make_shared<spdlog::logger>("CORE", sinks.begin(), sinks.end());
        s_CoreLogger->set_level(spdlog::level::trace);
        // Flush only on warnings+ — flushing the console sink on every trace/info
        // message forces a synchronous stdout flush each frame, which stalls the
        // editor badly when per-frame logs are active. spdlog still flushes
        // periodically on its own.
        s_CoreLogger->flush_on(spdlog::level::warn);

        s_ClientLogger = std::make_shared<spdlog::logger>("CLIENT", sinks.begin(), sinks.end());
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->flush_on(spdlog::level::warn);
    }

    static void SetLogLevel(LogLevel level)
    {
        spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
    }

    static void SetLogCallback(LogCallbackFn callback)
    {
        s_LogCallback = callback;
    }

    static std::vector<BufferedLogEntry> ConsumeBufferedMessages()
    {
        std::lock_guard<std::mutex> lock(s_BufferMutex);
        std::vector<BufferedLogEntry> messages(std::make_move_iterator(s_Buffer.begin()), std::make_move_iterator(s_Buffer.end()));
        s_Buffer.clear();
        return messages;
    }

    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    inline static std::shared_ptr<spdlog::logger> s_CoreLogger;
    inline static std::shared_ptr<spdlog::logger> s_ClientLogger;
    inline static LogCallbackFn s_LogCallback = nullptr;
    inline static std::deque<BufferedLogEntry> s_Buffer;
    inline static std::mutex s_BufferMutex;
    static constexpr size_t MAX_BUFFERED_MESSAGES = 10000;
};
} // namespace Chained

// Core logging macros
#ifdef CH_ENABLE_LOGGING
#define CH_CORE_TRACE(...)  ::Chained::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CH_CORE_INFO(...)   ::Chained::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CH_CORE_WARN(...)   ::Chained::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CH_CORE_ERROR(...)  ::Chained::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CH_CORE_FATAL(...)  ::Chained::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client logging macros
#define CH_TRACE(...)  ::Chained::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CH_INFO(...)   ::Chained::Log::GetClientLogger()->info(__VA_ARGS__)
#define CH_WARN(...)   ::Chained::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CH_ERROR(...)  ::Chained::Log::GetClientLogger()->error(__VA_ARGS__)
#define CH_FATAL(...)  ::Chained::Log::GetClientLogger()->critical(__VA_ARGS__)
#else
// Core logging macros
#define CH_CORE_TRACE(...)
#define CH_CORE_INFO(...)
#define CH_CORE_WARN(...)
#define CH_CORE_ERROR(...)
#define CH_CORE_FATAL(...)

// Client logging macros
#define CH_TRACE(...)
#define CH_INFO(...)
#define CH_WARN(...)
#define CH_ERROR(...)
#define CH_FATAL(...)
#endif

#endif // CH_LOG_H

