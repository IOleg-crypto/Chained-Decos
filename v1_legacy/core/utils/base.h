#ifndef CD_CORE_UTILS_BASE_H
#define CD_CORE_UTILS_BASE_H

// Platform detection
#ifdef _WIN32
#define CD_PLATFORM_WINDOWS
#elif defined(__linux__)
#define CD_PLATFORM_LINUX
#endif

#include "core/log.h"
#include <memory>
#include <utility>

enum class SceneType
{
    Game,
    UI
};

namespace CHEngine
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

} // namespace CHEngine

// Macros for events and assertions
#define CD_BIND_EVENT_FN(fn)                                                                       \
    [this](auto &&...args) -> decltype(auto)                                                       \
    { return this->fn(std::forward<decltype(args)>(args)...); }

#define CD_CORE_ASSERT(x, ...)                                                                     \
    if (!(x))                                                                                      \
    {                                                                                              \
        CD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__);                                       \
        __debugbreak();                                                                            \
    }

#endif // CD_CORE_UTILS_BASE_H
