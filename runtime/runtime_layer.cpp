#include "runtime_layer.h"
#include "core/application/application.h"
#include "core/audio/audio.h"
#include "core/input/input.h"
#include "core/log.h"
#include "core/physics/physics.h"
#include "core/renderer/renderer.h"
#include "events/key_event.h"
#include "scene/core/scene_manager.h"
#include "scene/ecs/components/skybox_component.h"
#include "scene/ecs/components/spawn_point_component.h"

// Services
#include "core/scripting/script_manager.h"
#include "events/ui_event_registry.h"
#include "logic/RuntimeInitializer.h"
#include "scene/core/scene.h"
#include "scene/ecs/components/physics_data.h"
#include "scene/ecs/components/player_component.h"
#include "scene/ecs/components/render_component.h"
#include "scene/ecs/components/transform_component.h"
#include "scene/ecs/components/utility_components.h"
#include "scene/ecs/components/velocity_component.h"
#include "scene/ecs/systems/entity_collision_system.h"
#include "scene/ecs/systems/lifetime_system.h"
#include "scene/ecs/systems/physics_system.h"
#include "scene/ecs/systems/player_system.h"
#include "scene/ecs/systems/render_system.h"
#include "scene/ecs/systems/skybox_system.h"
#include "scene/ecs/systems/ui_render_system.h"
#include <algorithm>
#include <raylib.h>
#include <raymath.h>
#include <vector>

using namespace CHEngine;
using namespace CHD;

namespace CHD
{

RuntimeLayer::RuntimeLayer(std::shared_ptr<CHEngine::Scene> scene)
    : Layer("RuntimeLayer"), m_Scene(scene)
{
}

void RuntimeLayer::OnAttach()
{
    // Register UI Events
    UIEventRegistry::Register(
        "start_game",
        [this]()
        {
            CD_INFO("[RuntimeLayer] Start Game Event Triggered!");
            auto scene = SceneManager::IsInitialized() ? SceneManager::GetActiveScene() : nullptr;
            if (!scene)
                return;
            auto view =
                scene->GetRegistry().view<TransformComponent, VelocityComponent, PlayerComponent>();
            for (auto [entity, transform, velocity, player] : view.each())
            {
                transform.position = player.spawnPosition;
                velocity.velocity = {0, 0, 0};
            }
        });

    UIEventRegistry::Register("quit_game",
                              [this]()
                              {
                                  CD_INFO("[RuntimeLayer] Quit Game Event Triggered!");
                                  Application::Get().Close();
                              });

    CD_INFO("RuntimeLayer Attached");

    m_hudFont = CHD::RuntimeInitializer::LoadHUDFont(m_fontLoaded);

    int locWindDir;
    m_playerShader =
        CHD::RuntimeInitializer::LoadPlayerShader(m_locFallSpeed, m_locTime, locWindDir);
    m_shaderLoaded = (m_playerShader.id != 0);

    auto scene = SceneManager::IsInitialized() ? SceneManager::GetActiveScene() : nullptr;
    if (scene && ScriptManager::IsInitialized())
        ScriptManager::InitializeScripts(scene->GetRegistry());
}

void RuntimeLayer::OnDetach()
{
    if (m_shaderLoaded)
    {
        UnloadShader(m_playerShader);
        m_shaderLoaded = false;
    }

    if (m_fontLoaded)
    {
        UnloadFont(m_hudFont);
        m_fontLoaded = false;
    }

    CD_INFO("RuntimeLayer Detached");
}

void RuntimeLayer::OnUpdate(float deltaTime)
{
    // Update Player Shader Uniforms
    if (m_shaderLoaded)
    {
        float time = (float)GetTime();
        SetShaderValue(m_playerShader, m_locTime, &time, SHADER_UNIFORM_FLOAT);

        if (SceneManager::IsInitialized())
        {
            auto scene = SceneManager::GetActiveScene();
            if (scene)
            {
                auto playerView = scene->GetRegistry().view<PlayerComponent, VelocityComponent>();
                for (auto &&[entity, player, velocity] : playerView.each())
                {
                    float fallSpeed =
                        (velocity.velocity.y < 0) ? std::abs(velocity.velocity.y) : 0.0f;
                    SetShaderValue(m_playerShader, m_locFallSpeed, &fallSpeed,
                                   SHADER_UNIFORM_FLOAT);
                }
            }
        }
    }

    // UPDATE C# SCRIPTS
    if (SceneManager::IsInitialized())
    {
        auto currentScene = SceneManager::GetActiveScene();
        if (currentScene && ScriptManager::IsInitialized())
        {
            ScriptManager::SetActiveRegistry(&currentScene->GetRegistry());
            ScriptManager::InitializeScripts(currentScene->GetRegistry());
            ScriptManager::UpdateScripts(currentScene->GetRegistry(), deltaTime);
        }
    }

    auto scene = SceneManager::GetActiveScene();
    if (!scene)
        return;

    // ECS SYSTEMS UPDATE
    PlayerSystem::Update(scene->GetRegistry(), deltaTime);
    PhysicsSystem::Update(scene->GetRegistry(), deltaTime);
    EntityCollisionSystem::Update(scene->GetRegistry(), deltaTime);
    LifetimeSystem::Update(scene->GetRegistry(), deltaTime);

    // Overlay Scripts
    if (SceneManager::IsInitialized())
    {
        auto uiScene = SceneManager::GetUIScene();
        if (uiScene && uiScene.get() != m_Scene.get() && ScriptManager::IsInitialized())
        {
            ScriptManager::SetActiveRegistry(&uiScene->GetRegistry());
            ScriptManager::UpdateScripts(uiScene->GetRegistry(), deltaTime);
        }
    }
}

void RuntimeLayer::OnRender()
{
    Renderer::BeginMode3D(Renderer::GetCamera());
    SkyboxSystem::Render(m_Scene->GetRegistry());
    RenderScene();
    Renderer::EndMode3D();
    RenderUI((float)GetScreenWidth(), (float)GetScreenHeight());
}

void RuntimeLayer::RenderUI(float width, float height)
{
    UIRenderSystem::Render(m_Scene->GetRegistry(), (int)width, (int)height);
    UIRenderSystem::RenderImGui(m_Scene->GetRegistry(), (int)width, (int)height);

    if (SceneManager::IsInitialized())
    {
        auto uiScene = SceneManager::GetUIScene();
        if (uiScene && uiScene.get() != m_Scene.get())
        {
            UIRenderSystem::Render(uiScene->GetRegistry(), (int)width, (int)height);
            UIRenderSystem::RenderImGui(uiScene->GetRegistry(), (int)width, (int)height);
        }
    }

    UIRenderSystem::RenderHUD(m_Scene->GetRegistry(), (int)width, (int)height);
}

void RuntimeLayer::RenderScene()
{
    RenderSystem::Render(m_Scene->GetRegistry());
}

void RuntimeLayer::OnEvent(Event &e)
{
}

} // namespace CHD
