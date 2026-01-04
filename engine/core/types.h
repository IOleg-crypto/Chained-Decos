#ifndef CH_TYPES_H
#define CH_TYPES_H

#include "engine/core/base.h"
#include "engine/core/log.h"
#include <cstdint>

namespace CH
{
// Assertions
#ifdef CH_ENABLE_ASSERTS
#define CH_ASSERT(x, ...)                                                                          \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CH_ERROR("Assertion Failed: %s", ##__VA_ARGS__);                                       \
            CH_DEBUGBREAK();                                                                       \
        }                                                                                          \
    }
#define CH_CORE_ASSERT(x, ...)                                                                     \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CH_CORE_ERROR("Assertion Failed: %s", ##__VA_ARGS__);                                  \
            CH_DEBUGBREAK();                                                                       \
        }                                                                                          \
    }
#else
#define CH_ASSERT(x, ...)
#define CH_CORE_ASSERT(x, ...)
#endif
} // namespace CH

#endif // CH_TYPES_H
