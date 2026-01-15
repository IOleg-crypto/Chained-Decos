#ifndef CH_COMPONENTS_H
#define CH_COMPONENTS_H

#include "components/animation_component.h"
#include "components/hierarchy_component.h"
#include "components/scripting_components.h"
#include "components/ui_component.h"
#include "engine/core/base.h"
#include "engine/core/math_types.h"
#include "engine/core/uuid.h"
#include "engine/renderer/material.h"
#include <future>
#include <raymath.h>
#include <string>

namespace CHEngine
{
struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const IDComponent &) = default;
    IDComponent(const UUID &id) : ID(id)
    {
    }
};

struct BVHNode;

struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent &) = default;
    TagComponent(const std::string &tag) : Tag(tag)
    {
    }
};

struct TransformComponent
{
    Vector3 Translation = {0.0f, 0.0f, 0.0f};
    Vector3 Rotation = {0.0f, 0.0f, 0.0f};
    Vector3 Scale = {1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const TransformComponent &) = default;
    TransformComponent(const Vector3 &translation) : Translation(translation)
    {
    }

    Matrix GetTransform() const
    {
        Matrix rotation = MatrixRotateXYZ(Rotation);

        return MatrixMultiply(MatrixMultiply(MatrixScale(Scale.x, Scale.y, Scale.z), rotation),
                              MatrixTranslate(Translation.x, Translation.y, Translation.z));
    }
};

struct ModelComponent
{
    std::string ModelPath;
    MaterialInstance Material;
    Vector3 Scale = {1.0f, 1.0f, 1.0f};

    ModelComponent() = default;
    ModelComponent(const ModelComponent &) = default;
    ModelComponent(const std::string &path) : ModelPath(path)
    {
    }
};

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
    Ref<BVHNode> BVHRoot = nullptr;
    std::shared_future<Ref<BVHNode>> BVHFuture;

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

struct SpawnComponent
{
    bool IsActive = true;
    Vector3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;
    Vector3 SpawnPoint = {0.0f, 0.0f, 0.0f};

    SpawnComponent() = default;
    SpawnComponent(const SpawnComponent &) = default;
};

// DEPRECATED: Material is now part of ModelComponent or separate MaterialInstance
// struct MaterialComponent ...

struct PointLightComponent
{
    Color LightColor = WHITE;
    float Radiance = 1.0f;
    float Radius = 10.0f;
    float Falloff = 1.0f;

    PointLightComponent() = default;
    PointLightComponent(const PointLightComponent &) = default;
};

struct SkyboxComponent
{
    std::string TexturePath;
    float Exposure = 1.0f;
    float Brightness = 0.0f;
    float Contrast = 1.0f;

    SkyboxComponent() = default;
    SkyboxComponent(const SkyboxComponent &) = default;
};

struct PlayerComponent
{
    float MovementSpeed = 15.0f;
    float LookSensitivity = 0.9f;

    // Camera Sync
    float CameraYaw = 0.0f;
    float CameraPitch = 20.0f;
    float CameraDistance = 10.0f;

    float JumpForce = 10.0f;

    PlayerComponent() = default;
};

struct AudioComponent
{
    std::string SoundPath;
    float Volume = 1.0f;
    float Pitch = 1.0f;
    bool Loop = false;
    bool PlayOnStart = true;
};

struct CameraComponent
{
    float Fov = 110.0f;
    Vector3 Offset = {0.0f, 2.0f, -5.0f}; // Offset from target
    Vector2 MousePosition = {0.0f, 0.0f};

    CameraComponent() = default;
};

} // namespace CHEngine

#endif // CH_COMPONENTS_H
