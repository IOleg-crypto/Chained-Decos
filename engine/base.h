#ifndef CH_BASE_H
#define CH_BASE_H

#include <memory>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define CH_PLATFORM_WINDOWS
#elif defined(__linux__)
#define CH_PLATFORM_LINUX
#else
#error "Unknown platform!"
#endif

// Debug/Release detection
#ifdef _DEBUG
#define CH_DEBUG
#else
#define CH_RELEASE
#endif

// Debug break
#ifdef CH_DEBUG
#if defined(CH_PLATFORM_WINDOWS)
#define CH_DEBUGBREAK() __debugbreak()
#elif defined(CH_PLATFORM_LINUX)
#include <signal.h>
#define CH_DEBUGBREAK() raise(SIGTRAP)
#else
#define CH_DEBUGBREAK()
#endif
#define CH_ENABLE_ASSERTS
#else
#define CH_DEBUGBREAK()
#endif

// Bit manipulation
#define BIT(x) (1 << (x))

// Bind event function helper
#define CH_BIND_EVENT_FN(fn)                                                                       \
    [this](auto &&...args) -> decltype(auto)                                                       \
    { return this->fn(std::forward<decltype(args)>(args)...); }

namespace CH
{
template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... Args> constexpr Scope<T> CreateScope(Args &&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr Ref<T> CreateRef(Args &&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}
} // namespace CH

#endif // CH_BASE_H
