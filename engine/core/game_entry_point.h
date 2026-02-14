#ifndef CH_GAME_ENTRY_POINT_H
#define CH_GAME_ENTRY_POINT_H

#ifdef CH_PLATFORM_WINDOWS
    #ifdef GAME_BUILD_DLL
        #define CH_GAME_API __declspec(dllexport)
    #else
        #define CH_GAME_API __declspec(dllimport)
    #endif
#else
    #define CH_GAME_API
#endif

// Forward declarations for ABI-safe script registration
namespace CHEngine {
    class ScriptableEntity;
    struct ScriptInstance;
}

// C-compatible function pointer types for script factories
typedef CHEngine::ScriptableEntity* (*CH_InstantiateFn)();
typedef void (*CH_DestroyFn)(CHEngine::ScriptInstance*);

// Callback that the Engine provides to the DLL for registering scripts.
// The DLL calls this for each script it wants to register.
// All std::unordered_map operations happen inside the EXE, not the DLL.
typedef void (*CH_RegisterScriptCallback)(
    void* userData,           // opaque pointer (ScriptRegistry*)
    const char* scriptName,   // plain C string - ABI safe
    CH_InstantiateFn instantiate,
    CH_DestroyFn destroy
);

extern "C" {
    // New ABI-safe signature: DLL receives a callback + opaque pointer
    CH_GAME_API void LoadGame(CH_RegisterScriptCallback registerCallback, void* userData);
    CH_GAME_API void UnloadGame();
}

// Helper macro for game scripts to simplify registration
#define CH_REGISTER_SCRIPT(callback, userData, ScriptClass, ScriptName) \
    callback(userData, ScriptName, \
        []() -> CHEngine::ScriptableEntity* { return static_cast<CHEngine::ScriptableEntity*>(new ScriptClass()); }, \
        [](CHEngine::ScriptInstance* si) { delete si->Instance; si->Instance = nullptr; })

#endif // CH_GAME_ENTRY_POINT_H
