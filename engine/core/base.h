#ifndef CH_BASE_H
#define CH_BASE_H

 #define GLM_ENABLE_EXPERIMENTAL
#include "engine/core/log.h"
#include "engine/core/ch_math.h"
#include <memory>

#include "engine/core/platform_detection.h"

// Debug/Release detection
#ifdef _DEBUG
#define CH_DEBUG
#else
#define CH_RELEASE
#endif

// Debug break
#ifdef CH_DEBUG
#if CH_PLATFORM_WINDOWS
#define CH_DEBUGBREAK() __debugbreak()
#elif CH_PLATFORM_LINUX
#include <csignal>
#define CH_DEBUGBREAK() std::raise(SIGTRAP)
#else
#define CH_DEBUGBREAK()
#endif
#define CH_ENABLE_ASSERTS
#else
#define CH_DEBUGBREAK()
#endif


// Macro helpers
#define CH_EXPAND_MACRO(x) x
#define CH_STRINGIFY_MACRO(x) #x

#ifdef CH_DEBUG
#define CH_ENABLE_ASSERTS
#endif



// Bit manipulation
#define BIT(x) (1 << (x))

// Bind event function helper
#define CH_BIND_EVENT_FN(fn)                                                                                           \
    [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


// --- Scripting & Reflection Macros ---
#define CH_CLASS(...)
#define CH_PROPERTY(...)
#define CH_FUNCTION(...)

#endif // CH_BASE_H

