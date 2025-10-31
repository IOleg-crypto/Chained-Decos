#ifndef GAME_RENDER_HELPER_H
#define GAME_RENDER_HELPER_H

#include <raylib.h>
#include <Engine/Collision/CollisionSystem.h>

class CollisionManager;

class GameRenderHelper
{
private:
    CollisionManager* m_collisionManager;

public:
    explicit GameRenderHelper(CollisionManager* collisionManager);
    ~GameRenderHelper() = default;

    void CreatePlatform(const Vector3 &position, const Vector3 &size, Color color, CollisionType collisionType);
    static float CalculateDynamicFontSize(float baseSize);
};

#endif // GAME_RENDER_HELPER_H

