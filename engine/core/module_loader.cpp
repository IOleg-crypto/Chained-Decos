#include "module_loader.h"
#include "engine/core/log.h"
#include "engine/core/platform_detection.h"
#include "engine/core/game_entry_point.h"
#include "engine/scene/script_registry.h"

#ifdef CH_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace CHEngine {

    static void* s_GameDLL = nullptr;
    static std::string s_LoadedDLLPath;

#ifdef CH_PLATFORM_WINDOWS
    // Cached function pointer from the loaded DLL
    typedef void (*LoadGameFn)(CH_RegisterScriptCallback, void*);
    static LoadGameFn s_LoadGameFn = nullptr;
#endif

    // This callback runs inside the EXE's code, so all map operations
    // use the EXE's std::unordered_map implementation — no ABI conflict.
    static void EngineRegisterScriptCallback(
        void* userData,
        const char* scriptName,
        CH_InstantiateFn instantiate,
        CH_DestroyFn destroy)
    {
        auto* registry = static_cast<ScriptRegistry*>(userData);
        registry->RegisterDirect(scriptName, instantiate, destroy);
    }

    bool ModuleLoader::LoadGameModule(const std::string& dllPath, ScriptRegistry* registry) {
        #ifdef CH_PLATFORM_WINDOWS
        // If same DLL is already loaded, just re-register scripts into new registry
        if (s_GameDLL && s_LoadedDLLPath == dllPath && s_LoadGameFn) {
            CH_CORE_INFO("Game Module already loaded, re-registering scripts: {}", dllPath);
            s_LoadGameFn(EngineRegisterScriptCallback, registry);
            return true;
        }

        // Different DLL or first load
        if (s_GameDLL) {
            UnloadGameModule();
        }

        s_GameDLL = LoadLibraryA(dllPath.c_str());
        if (!s_GameDLL) {
            CH_CORE_ERROR("Failed to load Game Module DLL: {}", dllPath);
            return false;
        }

        s_LoadGameFn = (LoadGameFn)GetProcAddress((HMODULE)s_GameDLL, "LoadGame");

        if (s_LoadGameFn) {
            s_LoadedDLLPath = dllPath;
            CH_CORE_INFO("Loaded Game Module: {}", dllPath);
            s_LoadGameFn(EngineRegisterScriptCallback, registry);
            return true;
        } else {
            CH_CORE_ERROR("Failed to find 'LoadGame' symbol in Game Module DLL");
            FreeLibrary((HMODULE)s_GameDLL);
            s_GameDLL = nullptr;
            s_LoadedDLLPath.clear();
            return false;
        }
        #else
        CH_CORE_ERROR("Dynamic loading not supported on this platform yet!");
        return false;
        #endif
    }

    void ModuleLoader::UnloadGameModule() {
        #ifdef CH_PLATFORM_WINDOWS
        if (s_GameDLL) {
            typedef void (*UnloadGameFn)();
            UnloadGameFn unloadGame = (UnloadGameFn)GetProcAddress((HMODULE)s_GameDLL, "UnloadGame");
            if (unloadGame) {
                unloadGame();
            }

            FreeLibrary((HMODULE)s_GameDLL);
            s_GameDLL = nullptr;
            s_LoadGameFn = nullptr;
            s_LoadedDLLPath.clear();
            CH_CORE_INFO("Unloaded Game Module");
        }
        #endif
    }
}
