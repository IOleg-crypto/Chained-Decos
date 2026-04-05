#ifndef CH_GAME_COMPONENTS_H
#define CH_GAME_COMPONENTS_H

#include "engine/core/reflection.h"

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

    CH_REFLECT_BEGIN(SpawnComponent)
        props.Property("Active", IsActive);
        props.Property("Zone Size", ZoneSize, PropertyMeta(0.1f, 100.0f, 0.1f));
        props.Handle("Texture Handle", TextureHandle);
        props.File("Texture Path", TexturePath, "png,jpg,bmp,tga");
        props.Property("Render Zone", RenderSpawnZoneInScene);
        props.Property("Spawn Point", SpawnPoint);
    CH_REFLECT_END()
};

struct PlayerComponent
{
    float MovementSpeed = 15.0f;
    float JumpForce = 10.0f;
    float LookSensitivity = 0.9f;

    PlayerComponent() = default;

    CH_REFLECT_BEGIN(PlayerComponent)
        props.Property("Movement Speed", MovementSpeed, PropertyMeta(0.1f, 100.0f, 0.5f));
        props.Property("Jump Force", JumpForce, PropertyMeta(0.1f, 50.0f, 0.5f));
        props.Property("Look Sensitivity", LookSensitivity, PropertyMeta(0.1f, 5.0f, 0.1f));
    CH_REFLECT_END()
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

    CH_REFLECT_BEGIN(SceneTransitionComponent)
        props.File("Target Scene", TargetScenePath, "chscene");
        props.Property("Triggered", Triggered);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_GAME_COMPONENTS_H
