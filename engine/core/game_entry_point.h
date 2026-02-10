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

namespace CHEngine {
    class Scene;
}

extern "C" {
    CH_GAME_API void LoadGame(CHEngine::Scene* scene);
    CH_GAME_API void UnloadGame();
}

#endif // CH_GAME_ENTRY_POINT_H
