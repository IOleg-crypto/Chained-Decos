#ifndef GAME_INITIALIZER_H
#define GAME_INITIALIZER_H

#include <entt/entt.hpp>
#include <raylib.h>

namespace CHEngine
{
class GameInitializer
{
public:
    static entt::entity InitializePlayer(Vector3 spawnPos, float sensitivity);
    static Shader LoadPlayerShader(int &locFallSpeed, int &locTime, int &locWindDir);
    static Font LoadHUDFont(bool &fontLoaded);
};
} // namespace CHEngine

#endif // GAME_INITIALIZER_H
