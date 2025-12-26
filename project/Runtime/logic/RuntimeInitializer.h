#ifndef RUNTIME_INITIALIZER_H
#define RUNTIME_INITIALIZER_H

#include <entt/entt.hpp>
#include <raylib.h>

namespace CHD
{
class RuntimeInitializer
{
public:
    static entt::entity InitializePlayer(Vector3 spawnPos, float sensitivity);
    static Shader LoadPlayerShader(int &locFallSpeed, int &locTime, int &locWindDir);
    static Font LoadHUDFont(bool &fontLoaded);
};
} // namespace CHD

#endif // RUNTIME_INITIALIZER_H
