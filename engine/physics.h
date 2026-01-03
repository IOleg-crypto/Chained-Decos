#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "scene.h"
#include <raylib.h>

namespace CH
{
class Physics
{
public:
    static void Init();
    static void Shutdown();

    static void Update(Scene *scene, float deltaTime);

    // Collision Detection Helpers
    static bool CheckAABB(const Vector3 &min1, const Vector3 &max1, const Vector3 &min2,
                          const Vector3 &max2);
};
} // namespace CH

#endif // CH_PHYSICS_H
