#ifndef CH_GAME_COMPONENTS_H
#define CH_GAME_COMPONENTS_H

#include "engine/core/base.h"
#include "engine/scene/components/control_component.h"
#include "raylib.h"
#include "../reflect.h"
#include <string>

namespace CHEngine
{
struct SpawnComponent
{
    bool IsActive = true;
    Vector3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;
    Vector3 SpawnPoint = {0.0f, 0.0f, 0.0f};

    std::string TexturePath =
        PROJECT_ROOT_DIR "/game/chaineddecos/assets/boxes/PlayerSpawnTexture.png";
    std::shared_ptr<TextureAsset> Texture;

    SpawnComponent() = default;
    SpawnComponent(const SpawnComponent &) = default;
};

BEGIN_REFLECT(SpawnComponent)
    PROPERTY(bool, IsActive, "Active")
    PROPERTY(Vector3, ZoneSize, "Zone Size")
    PROPERTY(bool, RenderSpawnZoneInScene, "Show In Scene")
    PROPERTY(Vector3, SpawnPoint, "Spawn Point")
    PROPERTY(std::string, TexturePath, "Texture Path")
END_REFLECT()

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

BEGIN_REFLECT(PlayerComponent)
    PROPERTY(float, MovementSpeed, "Speed")
    PROPERTY(float, LookSensitivity, "Sensitivity")
    PROPERTY(float, CameraYaw, "Yaw")
    PROPERTY(float, CameraPitch, "Pitch")
    PROPERTY(float, CameraDistance, "Cam Distance")
    PROPERTY(float, JumpForce, "Jump Force")
END_REFLECT()

struct SceneTransitionComponent
{
    std::string TargetScenePath;
    bool Triggered = false;

    SceneTransitionComponent() = default;
    SceneTransitionComponent(const std::string &path) : TargetScenePath(path)
    {
    }
};

BEGIN_REFLECT(SceneTransitionComponent)
    PROPERTY(std::string, TargetScenePath, "Target Scene")
    PROPERTY(bool, Triggered, "Triggered")
END_REFLECT()
} // namespace CHEngine

#endif // CH_GAME_COMPONENTS_H
