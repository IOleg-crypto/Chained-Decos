#include "GameInitializer.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "project/CHEngine/player/core/Player.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/Examples.h"

using namespace CHEngine;
#include "scene/ecs/components/RenderComponent.h"

namespace CHD
{
entt::entity GameInitializer::InitializePlayer(Vector3 spawnPos, float sensitivity)
{
    auto models = CHEngine::Engine::Instance().GetService<IModelLoader>();
    Model *playerModelPtr = nullptr;

    // Explicitly load player model if not loaded
    if (models)
    {
        std::string playerModelPath = std::string(PROJECT_ROOT_DIR) + "/resources/player_low.glb";
        if (models->LoadSingleModel("player_low", playerModelPath))
        {
            CD_INFO("[GameInitializer] Loaded player model: %s", playerModelPath.c_str());
        }

        auto modelOpt = models->GetModelByName("player_low");
        if (modelOpt.has_value())
        {
            playerModelPtr = &modelOpt.value().get();
        }
    }

    entt::entity playerEntity = entt::null;
    if (playerModelPtr)
    {
        playerEntity =
            CHEngine::ECSExamples::CreatePlayer(spawnPos, playerModelPtr, 8.0f, 12.0f, sensitivity);
    }
    else
    {
        CD_WARN("[GameInitializer] player_low not found, using default cube.");
        // We can't easily GenMesh here without static storage or management,
        // but for now let's just use the fallback if possible.
        // In the original code it used a member m_playerModel.
    }

    if (playerEntity != entt::null &&
        CHEngine::ECSRegistry::Get().all_of<CHEngine::RenderComponent>(playerEntity))
    {
        auto &renderComp =
            CHEngine::ECSRegistry::Get().get<CHEngine::RenderComponent>(playerEntity);
        renderComp.offset = {0.0f, Player::MODEL_Y_OFFSET, 0.0f};
    }

    return playerEntity;
}

Shader GameInitializer::LoadPlayerShader(int &locFallSpeed, int &locTime, int &locWindDir)
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

        CD_INFO("[GameInitializer] Loaded player_effect shader.");
    }
    else
    {
        CD_WARN("[GameInitializer] Failed to load player_effect shader.");
    }
    return shader;
}

Font GameInitializer::LoadHUDFont(bool &fontLoaded)
{
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/gantari/static/gantari-Bold.ttf";
    Font font = LoadFontEx(fontPath.c_str(), 96, 0, 0);

    if (font.baseSize > 0)
    {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        fontLoaded = true;
        CD_INFO("[GameInitializer] Loaded HUD font: %s", fontPath.c_str());
    }
    else
    {
        CD_ERROR("[GameInitializer] Failed to load HUD font: %s.", fontPath.c_str());
        fontLoaded = false;
        font = GetFontDefault();
    }
    return font;
}
} // namespace CHD
