#ifndef CH_COMPONENTS_H
#define CH_COMPONENTS_H

#include "components/hierarchy_component.h"
#include "components/scripting_components.h"
#include "components/ui_component.h"
#include "engine/core/base.h"
#include <raylib.h>
#include <raymath.h>
#include <string>

namespace CH
{
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
    Color Tint = WHITE;

    ModelComponent() = default;
    ModelComponent(const ModelComponent &) = default;
    ModelComponent(const std::string &path) : ModelPath(path)
    {
    }
};

struct BoxColliderComponent
{
    Vector3 Offset = {0.0f, 0.0f, 0.0f};
    Vector3 Size = {1.0f, 1.0f, 1.0f};

    // Collision state
    bool IsColliding = false;

    BoxColliderComponent() = default;
    BoxColliderComponent(const BoxColliderComponent &) = default;
};

struct SpawnComponent
{
    bool IsActive = true;
    Vector3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;

    SpawnComponent() = default;
    SpawnComponent(const SpawnComponent &) = default;
};

struct MaterialComponent
{
    Color AlbedoColor = WHITE;
    std::string AlbedoPath = "";

    MaterialComponent() = default;
    MaterialComponent(const MaterialComponent &) = default;
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
    Vector3 coordinates = {0.0f, 0.0f, 0.0f};

    // Physics & Camera Sync
    float CameraYaw = 0.0f;
    float CameraPitch = 20.0f;
    float CameraDistance = 10.0f;
    float VerticalVelocity = 0.0f;
    bool IsGrounded = false;

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

struct MeshColliderComponent
{
    std::string ModelPath;
    Ref<BVHNode> Root = nullptr;

    MeshColliderComponent() = default;
};

struct CameraComponent
{
    float Fov = 110.0f;
    Vector3 Offset = {0.0f, 2.0f, -5.0f}; // Offset from target
    Vector2 MousePosition = {0.0f, 0.0f};

    CameraComponent() = default;
};

struct WorldComponent
{
    // Jumping & Gravity
    float gravity = 30.0f;
    float jumpForce = 10.0f;
    float groundLevel = 0.0f; // Simple floor at Y=0 for now

    WorldComponent() = default;
};

} // namespace CH

#endif // CH_COMPONENTS_H
