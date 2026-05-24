#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/core/reflection_rfl.h"
#include "engine/physics/iphysics_world.h"
#include <glm/glm.hpp>
#include <string>

namespace CHEngine
{

enum class ColliderType { Box, Sphere, Capsule, Mesh };

struct ColliderComponent
{
    // Serialized fields
    ColliderType Type = ColliderType::Box;

    glm::vec3 Size = { 0.5f, 0.5f, 0.5f };
    glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
    float Radius = 0.5f;
    float Height = 1.0f;

    float Friction = 0.5f;
    float Restitution = 0.0f;

    bool IsTrigger = false;
    bool Enabled = true;
    bool AutoCalculate = false;

    std::string ModelPath;
    AssetHandle ModelHandle = 0;

    // Runtime state (not serialized - excluded by ReflectBridge)
    bool IsColliding = false;



    static const char* GetStaticName() { return "ColliderComponent"; }
};

CH_MARK_RFL(ColliderComponent);

struct RigidBodyComponent
{
    // Serialized fields
    enum class BodyType { Static, Dynamic, Kinematic };
    BodyType Type = BodyType::Static;
    float Mass = 1.0f;
    float LinearDamping = 0.01f;
    float AngularDamping = 0.05f;
    bool UseGravity = true;
    bool IsFixedRotation = false;
    bool IsKinematic = false;

    // Runtime state (not serialized - excluded by ReflectBridge)
    PhysicsBodyHandle Handle = kInvalidPhysicsBody;
    glm::vec3 Velocity = glm::vec3(0.0f);
    bool IsGrounded = false;



    static const char* GetStaticName() { return "RigidBodyComponent"; }
};

CH_MARK_RFL(RigidBodyComponent);

} // namespace CHEngine

#endif // CH_PHYSICS_COMPONENTS_H
