#include "RuntimeInitializer.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/assets/AssetManager.h"
#include "project/Runtime/player/Player.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UtilityComponents.h"
#include "scene/ecs/components/VelocityComponent.h"
#include "scene/ecs/components/playerComponent.h"

using namespace CHEngine;

namespace CHD
{
entt::entity RuntimeInitializer::InitializePlayer(Vector3 spawnPos, float sensitivity)
{
    Model *playerModelPtr = nullptr;

    // Explicitly load player model if not loaded
    std::string playerModelPath = std::string(PROJECT_ROOT_DIR) + "/resources/player_low.glb";
    if (AssetManager::LoadModel("player_low", playerModelPath))
    {
        CD_INFO("[RuntimeInitializer] Loaded player model: %s", playerModelPath.c_str());
    }

    auto modelOpt = AssetManager::GetModel("player_low");
    if (modelOpt)
    {
        playerModelPtr = &modelOpt->get();
    }

    // Create player entity directly (replacing ECSExamples::CreatePlayer)
    auto playerEntity = REGISTRY.create();

    if (!playerModelPtr)
    {
        CD_WARN("[RuntimeInitializer] player_low not found, creating player without model.");
    }

    // Transform
    REGISTRY.emplace<TransformComponent>(playerEntity,
                                         spawnPos,         // position
                                         Vector3{0, 0, 0}, // rotation
                                         Vector3{1, 1, 1}  // scale
    );

    // Velocity
    REGISTRY.emplace<VelocityComponent>(playerEntity);

    // Render (if model available)
    if (playerModelPtr)
    {
        auto &renderComp = REGISTRY.emplace<RenderComponent>(playerEntity,
                                                             "player",       // modelName
                                                             playerModelPtr, // model
                                                             GRAY,           // tint
                                                             true,           // visible
                                                             1               // renderLayer
        );
        renderComp.offset = {0.0f, ::Player::MODEL_Y_OFFSET, 0.0f};
    }

    // Player-specific component
    auto &pc = REGISTRY.emplace<PlayerComponent>(playerEntity,
                                                 8.0f,       // moveSpeed
                                                 12.0f,      // jumpForce
                                                 sensitivity // mouseSensitivity
    );
    pc.spawnPosition = spawnPos;

    // Physics
    REGISTRY.emplace<PhysicsData>(playerEntity,
                                  1.0f,  // mass
                                  -9.8f, // gravity
                                  true,  // useGravity
                                  false  // isKinematic
    );

    // Collision
    CHEngine::CollisionComponent collision;
    collision.bounds = BoundingBox{Vector3{-0.4f, 0.0f, -0.4f}, Vector3{0.4f, 1.8f, 0.4f}};
    collision.collisionLayer = 1; // Player layer
    REGISTRY.emplace<CHEngine::CollisionComponent>(playerEntity, collision);

    // Name for debugging
    REGISTRY.emplace<NameComponent>(playerEntity, "Player");

    return playerEntity;
}

Shader RuntimeInitializer::LoadPlayerShader(int &locFallSpeed, int &locTime, int &locWindDir)
{
    std::string vsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/player_effect.vs";
    std::string fsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/player_effect.fs";

    Shader shader = LoadShader(vsPath.c_str(), fsPath.c_str());
    if (shader.id != 0)
    {
        locFallSpeed = GetShaderLocation(shader, "fallSpeed");
        locTime = GetShaderLocation(shader, "time");
        locWindDir = GetShaderLocation(shader, "windDirection");

        float defaultFallSpeed = 0.0f;
        SetShaderValue(shader, locFallSpeed, &defaultFallSpeed, SHADER_UNIFORM_FLOAT);

        Vector3 defaultWind = {1.0f, 0.0f, 0.5f};
        SetShaderValue(shader, locWindDir, &defaultWind, SHADER_UNIFORM_VEC3);

        CD_INFO("[RuntimeInitializer] Loaded player_effect shader.");
    }
    else
    {
        CD_WARN("[RuntimeInitializer] Failed to load player_effect shader.");
    }
    return shader;
}

Font RuntimeInitializer::LoadHUDFont(bool &fontLoaded)
{
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/gantari/static/gantari-Bold.ttf";
    Font font = LoadFontEx(fontPath.c_str(), 96, 0, 0);

    if (font.baseSize > 0)
    {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        fontLoaded = true;
        CD_INFO("[RuntimeInitializer] Loaded HUD font: %s", fontPath.c_str());
    }
    else
    {
        CD_ERROR("[RuntimeInitializer] Failed to load HUD font: %s.", fontPath.c_str());
        fontLoaded = false;
        font = GetFontDefault();
    }
    return font;
}
} // namespace CHD
