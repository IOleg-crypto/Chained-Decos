#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/core/base.h"
#include <future>
#include "raylib.h"
#include <string>
#include "../reflect.h"

namespace CHEngine
{
class BVH;

enum class ColliderType : uint8_t
{
    Box = 0,
    Mesh = 1
};

struct ColliderComponent
{
    ColliderType Type = ColliderType::Box;
    bool Enabled = true;

    // Common/Box fields
    Vector3 Offset = {0.0f, 0.0f, 0.0f};
    Vector3 Size = {1.0f, 1.0f, 1.0f};
    bool AutoCalculate = true;

    // Mesh (BVH) fields
    std::string ModelPath;
    std::shared_ptr<BVH> BVHRoot = nullptr;

    bool IsColliding = false;

    ColliderComponent() = default;
    ColliderComponent(const ColliderComponent &) = default;
};

BEGIN_REFLECT(ColliderComponent)
    PROPERTY(bool, Enabled, "Enabled")
    PROPERTY(Vector3, Offset, "Offset")
    PROPERTY(Vector3, Size, "Size")
    PROPERTY(bool, AutoCalculate, "Auto Calculate")
    PROPERTY(std::string, ModelPath, "Model Path")
END_REFLECT()

struct RigidBodyComponent
{
    Vector3 Velocity = {0.0f, 0.0f, 0.0f};
    bool UseGravity = true;
    bool IsGrounded = false;
    bool IsKinematic = false;
    float Mass = 1.0f;

    RigidBodyComponent() = default;
};

BEGIN_REFLECT(RigidBodyComponent)
    PROPERTY(Vector3, Velocity, "Velocity")
    PROPERTY(bool, UseGravity, "Gravity")
    PROPERTY(bool, IsGrounded, "Grounded")
    PROPERTY(bool, IsKinematic, "Kinematic")
    PROPERTY(float, Mass, "Mass")
END_REFLECT()
} // namespace CHEngine

#endif // CH_PHYSICS_COMPONENTS_H
