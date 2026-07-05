#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/model_data.h"
#include "engine/physics/iphysics_world.h"
#include "engine/reflection/reflection_rfl.h"
#include <glm/glm.hpp>
#include <string>

namespace Chained
{

struct ColliderComponent
{
    // Serialized fields
    ColliderType Type = ColliderType::Box;

    glm::vec3 Size = {0.5f, 0.5f, 0.5f};
    glm::vec3 Offset = {0.0f, 0.0f, 0.0f};
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

    static const char* GetStaticName()
    {
        return "ColliderComponent";
    }
    static BoundingBox CalculateWorldAABB(const ColliderComponent& collider, const glm::mat4& worldTransform);

    struct UI
    {
        UIMeta Type = {.Tooltip = "Geometric shape of the collider", .Hint = PropertyMeta::WidgetHint::Enum};
        UIMeta Size = {.Speed = 0.05f, .Tooltip = "Dimensions of the Box collider"};
        UIMeta Offset = {.Speed = 0.05f,
                         .Tooltip = "Offset of the collider's center relative to the entity's local origin"};
        UIMeta Radius = {.Min = 0.0f, .Max = 500.0f, .Speed = 0.05f, .Tooltip = "Radius for Sphere or Capsule"};
        UIMeta Height = {.Min = 0.0f, .Max = 500.0f, .Speed = 0.05f, .Tooltip = "Total height of the capsule"};
        UIMeta Friction = {.Min = 0.0f, .Max = 1.0f, .Speed = 0.01f, .Tooltip = "Material friction coefficient"};
        UIMeta Restitution = {
            .Min = 0.0f, .Max = 1.0f, .Speed = 0.01f, .Tooltip = "Material restitution (bounciness on impact)"};
        UIMeta IsTrigger = {
            .Tooltip =
                "If enabled, the collider only registers intersections but does not block physical body movement"};
        UIMeta AutoCalculate = {.Tooltip = "Automatically calculate collider dimensions based on the ModelComponent"};
        UIMeta ModelPath = {.Hint = PropertyMeta::WidgetHint::FilePicker,
                            .Extensions = ".glb,.gltf,.obj",
                            .Tooltip = "Path to the 3D model for the Mesh collider"};
    };
};

CH_MARK_RFL(ColliderComponent);

struct RigidBodyComponent
{
    // Serialized fields
    enum class BodyType
    {
        Static,
        Dynamic,
        Kinematic
    };
    BodyType Type = BodyType::Dynamic;
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

    static const char* GetStaticName()
    {
        return "RigidBodyComponent";
    }

    struct UI
    {
        UIMeta Type = {
            .Tooltip =
                "Body behavior mode: Static (immovable), Dynamic (full physics), Kinematic (controlled by code)",
            .Hint = PropertyMeta::WidgetHint::Enum};
        UIMeta Mass = {.Min = 0.0f, .Max = 100000.0f, .Speed = 0.1f, .Tooltip = "Mass of the physical object"};
        UIMeta LinearDamping = {.Min = 0.0f,
                                .Max = 1.0f,
                                .Speed = 0.005f,
                                .Tooltip = "Air resistance for linear movement (linear damping)"};
        UIMeta AngularDamping = {.Min = 0.0f,
                                 .Max = 1.0f,
                                 .Speed = 0.005f,
                                 .Tooltip = "Air resistance for object rotation (angular damping)"};
        UIMeta UseGravity = {.Tooltip = "Whether global scene gravity affects this body"};
        UIMeta IsFixedRotation = {.Tooltip = "Blocks any rotation of the object caused by physical forces"};
        UIMeta IsKinematic = {.Tooltip = "Duplicate kinematic status flag for internal calculations"};
    };
};

CH_MARK_RFL(RigidBodyComponent);

} // namespace Chained

#endif // CH_PHYSICS_COMPONENTS_H