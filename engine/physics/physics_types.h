#ifndef CH_PHYSICS_TYPES_H
#define CH_PHYSICS_TYPES_H

#include <cstdint>

namespace Chained
{
    using PhysicsBodyHandle = uint64_t;
    constexpr PhysicsBodyHandle kInvalidPhysicsBody = 0;

    enum class ColliderType
    {
        Box,
        Sphere,
        Capsule,
        Mesh
    };
}

#endif // CH_PHYSICS_TYPES_H
