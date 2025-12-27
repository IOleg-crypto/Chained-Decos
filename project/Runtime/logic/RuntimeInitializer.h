#ifndef RUNTIME_INITIALIZER_H
#define RUNTIME_INITIALIZER_H

#include "scene/core/Entity.h"
#include "scene/core/Scene.h"
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

#endif // RUNTIME_INITIALIZER_H
