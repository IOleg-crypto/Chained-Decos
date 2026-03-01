#ifndef CH_GAME_COMPONENTS_H
#define CH_GAME_COMPONENTS_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "engine/scene/components/control_component.h"
#include "raylib.h"
#include <string>

namespace CHEngine
{
struct SpawnComponent
{
    bool IsActive = true;
    AssetHandle TextureHandle = 0;
    Vector3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;
    Vector3 SpawnPoint = {0.0f, 0.0f, 0.0f};

    std::string TexturePath = PROJECT_ROOT_DIR "/game/chaineddecos/assets/boxes/PlayerSpawnTexture.png";
    std::shared_ptr<TextureAsset> Texture;

    SpawnComponent() = default;
    SpawnComponent(const SpawnComponent&) = default;
};

struct PlayerComponent
{
    float MovementSpeed = 15.0f;
    float JumpForce = 10.0f;
    float LookSensitivity = 0.9f;

    PlayerComponent() = default;
};

struct SceneTransitionComponent
{
    std::string TargetScenePath;
    bool Triggered = false;

    SceneTransitionComponent() = default;
    SceneTransitionComponent(const std::string& path)
        : TargetScenePath(path)
    {
    }
};

} // namespace CHEngine

#endif // CH_GAME_COMPONENTS_H
