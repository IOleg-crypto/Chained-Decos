#ifndef CD_RUNTIME_LOGIC_RUNTIMEINITIALIZER_H
#define CD_RUNTIME_LOGIC_RUNTIMEINITIALIZER_H

#include "engine/scene/core/entity.h"
#include "engine/scene/core/scene.h"
#include <raylib.h>


namespace CHD
{
class RuntimeInitializer
{
public:
    static CHEngine::Entity InitializePlayer(CHEngine::Scene *scene, Vector3 spawnPos,
                                             float sensitivity);
    static Shader LoadPlayerShader(int &locFallSpeed, int &locTime, int &locWindDir);
    static Font LoadHUDFont(bool &fontLoaded);
};
} // namespace CHD

#endif // CD_RUNTIME_LOGIC_RUNTIMEINITIALIZER_H
