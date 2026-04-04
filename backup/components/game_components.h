#ifndef CH_GAME_COMPONENTS_H
#define CH_GAME_COMPONENTS_H

#include "engine/core/base.h"
#include "engine/core/assets/asset.h"
#include "engine/scene/components/control_component.h"
#include <string>
#include <memory>

namespace CHEngine
{
struct SpawnComponent
{
    bool IsActive = true;
    AssetHandle TextureHandle = 0;
    glm::vec3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;
    glm::vec3 SpawnPoint = {0.0f, 0.0f, 0.0f};

    std::string TexturePath = PROJECT_ROOT_DIR "/game/chaineddecos/assets/boxes/PlayerSpawnTexture.png";
    std::shared_ptr<TextureAsset> Texture;

    SpawnComponent() = default;
    SpawnComponent(const SpawnComponent&) = default;

    static const char* GetStaticName() { return "SpawnComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, SpawnComponent& component)
    {
        archive.Property("SpawnZoneSize", component.ZoneSize)
            .Handle("SpawnTextureHandle", component.TextureHandle)
            .Path("SpawnTexturePath", component.TexturePath)
            .Property("RenderSpawnZoneInScene", component.RenderSpawnZoneInScene);
    }
};

struct PlayerComponent
{
    float MovementSpeed = 15.0f;
    float JumpForce = 10.0f;
    float LookSensitivity = 0.9f;

    PlayerComponent() = default;

    static const char* GetStaticName() { return "PlayerComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, PlayerComponent& component)
    {
        archive.Property("MovementSpeed", component.MovementSpeed)
            .Property("LookSensitivity", component.LookSensitivity)
            .Property("JumpForce", component.JumpForce);
    }
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

    static const char* GetStaticName() { return "SceneTransitionComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, SceneTransitionComponent& component)
    {
        archive.Property("TargetScenePath", component.TargetScenePath)
            .Property("Triggered", component.Triggered);
    }
};

} // namespace CHEngine

#endif // CH_GAME_COMPONENTS_H
