#include "module_loader.h"
#include "engine/core/log.h"
#include "engine/core/platform_detection.h"

#ifdef CH_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace CHEngine {

    static void* s_GameDLL = nullptr;

    bool ModuleLoader::LoadGameModule(const std::string& dllPath, Scene* scene) {
        if (s_GameDLL) {
            UnloadGameModule();
        }

        #ifdef CH_PLATFORM_WINDOWS
        // Copy DLL to a temp location to allow hot reloading (game.dll -> game_temp.dll)
        // For now, just load directly.
        
        s_GameDLL = LoadLibraryA(dllPath.c_str());
        if (!s_GameDLL) {
            CH_CORE_ERROR("Failed to load Game Module DLL: {}", dllPath);
            return false;
        }

        typedef void (*LoadGameFn)(Scene*);
        LoadGameFn loadGame = (LoadGameFn)GetProcAddress((HMODULE)s_GameDLL, "LoadGame");

        if (loadGame) {
            CH_CORE_INFO("Loaded Game Module: {}", dllPath);
            loadGame(scene);
            return true;
        } else {
            CH_CORE_ERROR("Failed to find 'LoadGame' symbol in Game Module DLL");
            FreeLibrary((HMODULE)s_GameDLL);
            s_GameDLL = nullptr;
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
            CH_CORE_INFO("Unloaded Game Module");
        }
        #endif
    }

}
