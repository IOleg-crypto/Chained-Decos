#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/core/base.h"
#include "engine/core/reflection.h"
#include <glm/glm.hpp>
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
    glm::vec3 Offset = {0.0f, 0.0f, 0.0f};

    // Box
    glm::vec3 Size = {1.0f, 1.0f, 1.0f};

    // Capsule
    float Radius = 0.5f;
    float Height = 2.0f;

    bool AutoCalculate = true;

    // Mesh (BVH) fields
    AssetHandle ModelHandle = 0;
    std::string ModelPath;

    bool IsColliding = false;

    ColliderComponent() = default;
    ColliderComponent(const ColliderComponent&) = default;

    CH_REFLECT_BEGIN(ColliderComponent)
        if (props.BeginGroup("General"))
        {
            const char* colliderTypes[] = {"Box", "Mesh", "Capsule", "Sphere"};
            props.Property("Type", Type, colliderTypes, 4);
            props.Property("Enabled", Enabled);
            props.Property("Offset", Offset);
            props.EndGroup();
        }
        
        if (props.BeginGroup("Shape Parameters"))
        {
            if (Type == ColliderType::Box)
            {
                props.Property("Size", Size, PropertyMeta(0.01f, 100.0f, 0.1f));
            }
            else if (Type == ColliderType::Capsule)
            {
                props.Property("Radius", Radius, PropertyMeta(0.01f, 50.0f, 0.1f));
                props.Property("Height", Height, PropertyMeta(0.1f, 100.0f, 0.1f));
            }
            else if (Type == ColliderType::Sphere)
            {
                props.Property("Radius", Radius, PropertyMeta(0.01f, 50.0f, 0.1f));
            }
            else if (Type == ColliderType::Mesh)
            {
                props.Handle("Model Handle", ModelHandle);
                props.File("Model Path", ModelPath, "obj,gltf,glb");
            }
            props.EndGroup();
        }
        
        props.Property("Auto Calculate", AutoCalculate);
    CH_REFLECT_END()
};

struct RigidBodyComponent
{
    glm::vec3 Velocity = {0.0f, 0.0f, 0.0f};
    bool UseGravity = true;
    bool IsGrounded = false;
    bool IsKinematic = false;
    float Mass = 1.0f;

    RigidBodyComponent() = default;

    CH_REFLECT_BEGIN(RigidBodyComponent)
        props.Header("Dynamics");
        props.Property("Mass", Mass, PropertyMeta(0.01f, 100.0f, 0.1f));
        props.Property("Velocity", Velocity);
        
        if (props.BeginGroup("State"))
        {
            props.Property("Use Gravity", UseGravity);
            props.Property("Is Kinematic", IsKinematic);
            props.Property("Is Grounded", IsGrounded);
            props.EndGroup();
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_PHYSICS_COMPONENTS_H
