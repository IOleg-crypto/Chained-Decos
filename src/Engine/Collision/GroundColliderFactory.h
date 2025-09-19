#ifndef GROUND_COLLIDER_FACTORY_H
#define GROUND_COLLIDER_FACTORY_H

#include "CollisionSystem.h"
#include <raylib.h>

class GroundColliderFactory
{
public:
    static Collision CreateAabbGround(const Vector3& center, const Vector3& halfSize);
    static Collision CreateDefaultGameGround();
};

#endif
