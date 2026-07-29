// script_interop_pointers.h
// Minimal struct for C# → C++ lifecycle callbacks.
// C++ → C# function pointers are registered via Coral's AddInternalCall
// (one AddInternalCall per function, no struct needed).
#ifndef CH_SCRIPT_INTEROP_POINTERS_H
#define CH_SCRIPT_INTEROP_POINTERS_H

#include <stdint.h>

namespace Chained
{
// Callbacks that C# invokes on C++ each frame / on events.
// Kept as a struct so C# can pass them in one call.
struct ManagedCallbacks
{
    void (*OnUpdate)(float);
    void (*OnEvent)(int);
    void (*OnRenderUI)();
    void (*OnCollisionEnter)(uint64_t, uint64_t);
    void (*ClearAll)();
    void (*InstantiateScript)(uint64_t, const char16_t*);
    void (*DestroyScript)(uint64_t, const char16_t*);
};

// Global callback pointers — set once during assembly load.
namespace ManagedCallbacks_
{
inline void (*OnUpdate)(float) = nullptr;
inline void (*OnEvent)(int) = nullptr;
inline void (*OnRenderUI)() = nullptr;
inline void (*OnCollisionEnter)(uint64_t, uint64_t) = nullptr;
inline void (*ClearAll)() = nullptr;
inline void (*InstantiateScript)(uint64_t, const char16_t*) = nullptr;
inline void (*DestroyScript)(uint64_t, const char16_t*) = nullptr;
} // namespace ManagedCallbacks_
} // namespace Chained

#endif // CH_SCRIPT_INTEROP_POINTERS_H
