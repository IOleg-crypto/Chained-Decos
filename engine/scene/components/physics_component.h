#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/core/reflection.h"
#include "engine/physics/iphysics_world.h"
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
        CH_HEADER(props, "Shape Selection");
        if (CH_BEGIN_GROUP(props, "General", true))
        {
            const char* colliderTypes[] = {"Box", "Mesh", "Capsule", "Sphere"};
            CH_ENUM(props, Type, colliderTypes);
            CH_PROP(props, Enabled);
            CH_PROP(props, Offset);
            CH_END_GROUP(props);
        }
        
        if (CH_BEGIN_GROUP(props, "ShapeParameters", true))
        {
            if (Type == ColliderType::Box)
            {
                CH_PROP_META(props, Size, PropertyMeta(0.01f, 100.0f, 0.1f));
            }
            else if (Type == ColliderType::Capsule)
            {
                CH_PROP_META(props, Radius, PropertyMeta(0.01f, 50.0f, 0.1f));
                CH_PROP_META(props, Height, PropertyMeta(0.1f, 100.0f, 0.1f));
            }
            else if (Type == ColliderType::Sphere)
            {
                CH_PROP_META(props, Radius, PropertyMeta(0.01f, 50.0f, 0.1f));
            }
            else if (Type == ColliderType::Mesh)
            {
                if (props.GetMode() != CHEngine::ReflectionMode::UI)
                    CH_HANDLE(props, ModelHandle);
                if (CH_FILE(props, ModelPath, "obj,gltf,glb"))
                {
                    ModelHandle = AssetHandle(0);
                }
            }
            CH_END_GROUP(props);
        }
        
        CH_PROP_META(props, AutoCalculate, PropertyMeta().WithTooltip("Overwrite Offset/Size automatically from model mesh each frame"));
    CH_REFLECT_END()
};

struct RigidBodyComponent
{
    glm::vec3 Velocity = {0.0f, 0.0f, 0.0f};
    bool UseGravity = true;
    bool IsGrounded = false;
    bool IsKinematic = false;
    float Mass = 1.0f;

    // Runtime handle
    PhysicsBodyHandle Handle = kInvalidPhysicsBody;

    CH_REFLECT_BEGIN(RigidBodyComponent)
        CH_HEADER(props, "Dynamics");
        CH_PROP_META(props, Mass, PropertyMeta(0.01f, 100.0f, 0.1f));
        CH_PROP(props, Velocity);
        
        if (CH_BEGIN_GROUP(props, "State", true))
        {
            CH_PROP(props, UseGravity);
            CH_PROP(props, IsKinematic);
            CH_PROP(props, IsGrounded);
            CH_END_GROUP(props);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_PHYSICS_COMPONENTS_H
