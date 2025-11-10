#ifndef GAME_RENDER_HELPERS_H
#define GAME_RENDER_HELPERS_H

#include <raylib.h>
#include <Engine/Collision/System/CollisionSystem.h>
#include "Engine/Collision/Manager/CollisionManager.h"

class GameRenderHelpers
{
private:
    CollisionManager* m_collisionManager;

public:
    explicit GameRenderHelpers(CollisionManager* collisionManager);
    ~GameRenderHelpers() = default;

    void CreatePlatform(const Vector3 &position, const Vector3 &size, Color color, CollisionType collisionType);
    static float CalculateDynamicFontSize(float baseSize);
};

#endif // GAME_RENDER_HELPERS_H

