#ifndef RENDER_HELPER_H
#define RENDER_HELPER_H

#include <raylib.h>
#include <Engine/Collision/CollisionSystem.h>

class CollisionManager;

class RenderHelper
{
private:
    CollisionManager* m_collisionManager;

public:
    explicit RenderHelper(CollisionManager* collisionManager);
    ~RenderHelper() = default;

    void CreatePlatform(const Vector3 &position, const Vector3 &size, Color color, CollisionType collisionType);
    static float CalculateDynamicFontSize(float baseSize);
};

#endif // RENDER_HELPER_H

