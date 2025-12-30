#pragma once
#include "core/Log.h"

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define CD_PLATFORM_WINDOWS
#ifdef _WIN64
#define CD_PLATFORM_WIN64
#else
#define CD_PLATFORM_WIN32
#endif
#elif defined(__linux__)
#define CD_PLATFORM_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define CD_PLATFORM_MACOS
#endif
#else
#error "Unknown platform!"
#endif

// Build configuration
#ifdef _DEBUG
#define CD_DEBUG
#else
#define CD_RELEASE
#endif

// Compiler detection
#if defined(_MSC_VER)
#define CD_COMPILER_MSVC
#elif defined(__clang__)
#define CD_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define CD_COMPILER_GCC
#endif

// DLL export/import (for future use if building as DLL)
#ifdef CD_PLATFORM_WINDOWS
#ifdef CD_BUILD_DLL
#define CD_API __declspec(dllexport)
#else
#define CD_API __declspec(dllimport)
#endif
#else
#define CD_API
#endif

// Assertions
#ifdef CD_DEBUG
#if defined(CD_PLATFORM_WINDOWS)
#define CD_DEBUGBREAK() __debugbreak()
#elif defined(CD_PLATFORM_LINUX) || defined(CD_PLATFORM_MACOS)
#include <signal.h>
#define CD_DEBUGBREAK() raise(SIGTRAP)
#else
#define CD_DEBUGBREAK()
#endif

#define CD_ENABLE_ASSERTS
#else
#define CD_DEBUGBREAK()
#endif

#ifdef CD_ENABLE_ASSERTS
#define CD_ASSERT(x, ...)                                                                          \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CD_ERROR("Assertion Failed: {0}", __VA_ARGS__);                                        \
            CD_DEBUGBREAK();                                                                       \
        }                                                                                          \
    }
#define CD_CORE_ASSERT(x, ...)                                                                     \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__);                                   \
            CD_DEBUGBREAK();                                                                       \
        }                                                                                          \
    }
#else
#define CD_ASSERT(x, ...)
#define CD_CORE_ASSERT(x, ...)
#endif

// Bit manipulation
#define BIT(x) (1 << x)

// Bind event function
#define CD_BIND_EVENT_FN(fn)                                                                       \
    [this](auto &&...args) -> decltype(auto)                                                       \
    { return this->fn(std::forward<decltype(args)>(args)...); }
