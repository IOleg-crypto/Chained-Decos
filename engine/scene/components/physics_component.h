#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "raylib.h"
#include <future>
#include <string>

namespace CHEngine
{
class BVH;

enum class ColliderType : uint8_t
{
    Box = 0,
    Mesh = 1,
    Capsule = 2,
    Sphere = 3
};

struct ColliderComponent
{
    ColliderType Type = ColliderType::Box;
    bool Enabled = true;

    // Common/Box fields
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    // Box
    Vector3 Size = {1.0f, 1.0f, 1.0f};

    // Capsule
    float Radius = 0.5f;
    float Height = 2.0f;

    bool AutoCalculate = true;

    // Mesh (BVH) fields
    AssetHandle ModelHandle = 0;
    std::string ModelPath;
    std::shared_ptr<BVH> BVHRoot = nullptr;

    bool IsColliding = false;

    ColliderComponent() = default;
    ColliderComponent(const ColliderComponent&) = default;
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
