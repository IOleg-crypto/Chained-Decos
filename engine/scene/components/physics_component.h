#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/core/base.h"
#include <future>
#include <raylib.h>
#include <string>

namespace CHEngine
{
struct BVHNode;

enum class ColliderType : uint8_t
{
    Box = 0,
    Mesh = 1
};

struct ColliderComponent
{
    ColliderType Type = ColliderType::Box;
    bool bEnabled = true;

    // Common/Box fields
    Vector3 Offset = {0.0f, 0.0f, 0.0f};
    Vector3 Size = {1.0f, 1.0f, 1.0f};
    bool bAutoCalculate = true;

    // Mesh (BVH) fields
    std::string ModelPath;
    std::shared_ptr<BVHNode> BVHRoot = nullptr;

    bool IsColliding = false;

    ColliderComponent() = default;
    ColliderComponent(const ColliderComponent &) = default;
};

struct RigidBodyComponent
{
    Vector3 Velocity = {0.0f, 0.0f, 0.0f};
    bool UseGravity = true;
    bool IsGrounded = false;
    bool IsKinematic = false;
    float Mass = 1.0f;

    RigidBodyComponent() = default;
};
} // namespace CHEngine

#endif // CH_PHYSICS_COMPONENTS_H
