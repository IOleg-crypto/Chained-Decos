#ifndef CH_BASE_H
#define CH_BASE_H

#define GLM_ENABLE_EXPERIMENTAL
#include "engine/core/log.h"
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
#include "signal.h"
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

namespace CHEngine
{
// Assertions
#ifdef CH_ENABLE_ASSERTS
#define CH_ASSERT(x, ...)                                                                          \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CH_ERROR("Assertion Failed: {0}", ##__VA_ARGS__);                                      \
            CH_DEBUGBREAK();                                                                       \
        }                                                                                          \
    }
#define CH_CORE_ASSERT(x, ...)                                                                     \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CH_CORE_ERROR("Assertion Failed: {0}", ##__VA_ARGS__);                                 \
            CH_DEBUGBREAK();                                                                       \
        }                                                                                          \
    }
#else
#define CH_ASSERT(x, ...)
#define CH_CORE_ASSERT(x, ...)
#endif

} // namespace CHEngine

#endif // CH_BASE_H
