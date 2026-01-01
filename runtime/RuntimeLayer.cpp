#include "RuntimeLayer.h"
#include "components/physics/collision/colsystem/CollisionSystem.h"
#include "core/Log.h"
#include "core/application/Application.h"
#include "core/audio/Audio.h"
#include "core/input/Input.h"
#include "core/interfaces/ILevelManager.h"
#include "core/physics/Physics.h"
#include "core/renderer/Renderer.h"
#include "events/KeyEvent.h"
#include "scene/core/SceneManager.h"
#include "scene/main/LevelManager.h"

// Services
#include "core/scripting/ScriptManager.h"
#include "events/UIEventRegistry.h"
#include "logic/RuntimeInitializer.h"
#include "scene/core/SceneManager.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/PlayerComponent.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UtilityComponents.h"
#include "scene/ecs/components/VelocityComponent.h"
#include "scene/ecs/systems/UIRenderSystem.h"
#include "scene/resources/map/GameScene.h"
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
            // Example logic: Reset player position
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

    // Load HUD Font via Initializer
    m_hudFont = CHD::RuntimeInitializer::LoadHUDFont(m_fontLoaded);

    // Load Player Shader via Initializer
    int locWindDir;
    m_playerShader =
        CHD::RuntimeInitializer::LoadPlayerShader(m_locFallSpeed, m_locTime, locWindDir);
    m_shaderLoaded = (m_playerShader.id != 0);

    // Initialize Scripts
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

        // Use SceneManager
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

    // UPDATE LUA SCRIPTS (Hazel style)
    if (SceneManager::IsInitialized())
    {
        auto currentScene = SceneManager::GetActiveScene();
        if (currentScene && ScriptManager::IsInitialized())
        {
            ScriptManager::SetActiveRegistry(&currentScene->GetRegistry());
            ScriptManager::UpdateScripts(currentScene->GetRegistry(), deltaTime);
        }
    }

    // Sync ECS Transforms back to MapObjects for rendering consistency
    if (LevelManager::IsInitialized())
    {
        LevelManager::SyncEntitiesToMap();
    }

    // 1. UPDATE PLAYER LOGIC (Previously PlayerSystem::Update)
    // Physics Update
    // Use SceneManager
    auto scene = SceneManager::GetActiveScene();
    if (!scene)
        return;

    auto playerView =
        scene->GetRegistry().view<TransformComponent, VelocityComponent, PlayerComponent>();

    for (auto [entity, transform, velocity, player] : playerView.each())
    {

        // Update Stats
        player.runTimer += deltaTime;
        if (transform.position.y > player.maxHeight)
            player.maxHeight = transform.position.y;

        // Handle Movement
        Vector2 mouseDelta = Input::GetMouseDelta();

        // Debug mouse delta if it's non-zero
        if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
        {
            CD_TRACE("[RuntimeLayer] Mouse Delta: %.2f, %.2f | Sensitivity: %.3f", mouseDelta.x,
                     mouseDelta.y, player.mouseSensitivity);
        }

        player.cameraDistance -= Input::GetMouseWheelMove() * 1.5f;
        player.cameraDistance = Clamp(player.cameraDistance, 2.0f, 20.0f);
        player.cameraYaw -= mouseDelta.x * player.mouseSensitivity;
        player.cameraPitch =
            Clamp(player.cameraPitch - mouseDelta.y * player.mouseSensitivity, -85.0f, 85.0f);

        Vector3 moveDir = {0, 0, 0};
        float yawRad = player.cameraYaw * DEG2RAD;
        Vector3 forward = Vector3Normalize({-sinf(yawRad), 0.0f, -cosf(yawRad)});
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));

        if (Input::IsKeyDown(KEY_W))
            moveDir = Vector3Add(moveDir, forward);
        if (Input::IsKeyDown(KEY_S))
            moveDir = Vector3Subtract(moveDir, forward);
        if (Input::IsKeyDown(KEY_D))
            moveDir = Vector3Add(moveDir, right);
        if (Input::IsKeyDown(KEY_A))
            moveDir = Vector3Subtract(moveDir, right);

        float moveLen = Vector3Length(moveDir);
        if (moveLen > 0.0f)
        {
            moveDir = Vector3Scale(moveDir, 1.0f / moveLen);
            float targetAngle = atan2f(moveDir.x, moveDir.z);

            // Smoother rotation
            float rotationSpeed = 10.0f;
            transform.rotation.y = transform.rotation.y +
                                   (targetAngle - transform.rotation.y) * rotationSpeed * deltaTime;
        }

        float targetSpeed = player.moveSpeed;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT) && player.isGrounded)
            targetSpeed *= 1.8f;

        if (player.isGrounded)
        {
            velocity.velocity.x = moveDir.x * targetSpeed;
            velocity.velocity.z = moveDir.z * targetSpeed;
        }
        else
        {
            float airControl = 0.3f;
            velocity.velocity.x += moveDir.x * targetSpeed * airControl * deltaTime;
            velocity.velocity.z += moveDir.z * targetSpeed * airControl * deltaTime;
            float currentSpeed = sqrtf(velocity.velocity.x * velocity.velocity.x +
                                       velocity.velocity.z * velocity.velocity.z);
            if (currentSpeed > targetSpeed)
            {
                velocity.velocity.x *= targetSpeed / currentSpeed;
                velocity.velocity.z *= targetSpeed / currentSpeed;
            }
        }

        // Jump
        if (Input::IsKeyPressed(KEY_SPACE) && player.isGrounded)
        {
            velocity.velocity.y = player.jumpForce;
            player.isGrounded = false;
        }

        // Physics Integration
        if (m_Scene->GetRegistry().all_of<PhysicsData>(entity))
        {
            auto &physics = m_Scene->GetRegistry().get<PhysicsData>(entity);
            if (physics.useGravity && !physics.isKinematic && !player.isGrounded)
                velocity.acceleration.y = physics.gravity;
            else
                velocity.acceleration.y = 0;
        }

        velocity.velocity =
            Vector3Add(velocity.velocity, Vector3Scale(velocity.acceleration, deltaTime));
        Vector3 proposedPos =
            Vector3Add(transform.position, Vector3Scale(velocity.velocity, deltaTime));

        // World Collision
        if (m_Scene->GetRegistry().all_of<CollisionComponent>(entity))
        {
            auto &collision = m_Scene->GetRegistry().get<CollisionComponent>(entity);
            Vector3 center = proposedPos;
            center.x += (collision.bounds.max.x + collision.bounds.min.x) * 0.5f;
            center.y += (collision.bounds.max.y + collision.bounds.min.y) * 0.5f;
            center.z += (collision.bounds.max.z + collision.bounds.min.z) * 0.5f;

            Vector3 halfSize =
                Vector3Scale(Vector3Subtract(collision.bounds.max, collision.bounds.min), 0.5f);
            CHEngine::Collision playerCol(center, halfSize);
            Vector3 response = {0};

            if (CHEngine::Physics::CheckCollision(playerCol, response))
            {
                proposedPos = Vector3Add(proposedPos, response);
                float resLen = Vector3Length(response);
                if (resLen > 0.001f)
                {
                    Vector3 normal = Vector3Scale(response, 1.0f / resLen);
                    float dot = Vector3DotProduct(velocity.velocity, normal);
                    if (dot < 0)
                        velocity.velocity =
                            Vector3Subtract(velocity.velocity, Vector3Scale(normal, dot));
                }
            }

            // Ground Check
            Vector3 rayOrigin = proposedPos;
            rayOrigin.y += 1.0f;
            float hitDist = 0;
            Vector3 hp, hn;
            if (Physics::RaycastDown(rayOrigin, 1.2f, hitDist, hp, hn))
            {
                if (velocity.velocity.y <= 0 && (hitDist - 1.0f) <= 0.1f)
                {
                    player.isGrounded = true;
                    proposedPos.y = hp.y;
                    velocity.velocity.y = 0;
                }
                else
                    player.isGrounded = false;
            }
            else
                player.isGrounded = false;
        }

        transform.position = proposedPos;

        // Drag
        float dragFactor = 1.0f - (velocity.drag * deltaTime);
        if (dragFactor < 0)
            dragFactor = 0;
        velocity.velocity.x *= dragFactor;
        velocity.velocity.z *= dragFactor;

        // Audio Update
        const float fallThresh = -5.0f;
        if (velocity.velocity.y < fallThresh && !player.isFallingSoundPlaying)
        {
            Audio::PlayLoopingSoundEffect("player_fall", 1.0f);
            player.isFallingSoundPlaying = true;
        }
        else if ((velocity.velocity.y >= fallThresh || player.isGrounded) &&
                 player.isFallingSoundPlaying)
        {
            Audio::StopLoopingSoundEffect("player_fall");
            player.isFallingSoundPlaying = false;
        }

        // Camera Update (Hazel-style)
        Camera3D camera = Renderer::GetCamera();
        float yawRadLocal = player.cameraYaw * DEG2RAD;
        float pitchRadLocal = player.cameraPitch * DEG2RAD;

        Vector3 camOffset = {player.cameraDistance * cosf(pitchRadLocal) * sinf(yawRadLocal),
                             player.cameraDistance * sinf(pitchRadLocal),
                             player.cameraDistance * cosf(pitchRadLocal) * cosf(yawRadLocal)};

        camera.target = Vector3Add(transform.position, {0, 1.5f, 0});
        camera.position = Vector3Add(camera.target, camOffset);
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        // Apply camera update to renderer
        Renderer::SetCamera(camera);
    }

    // 2. UPDATE ENTITY COLLISIONS (Previously EntityCollisionSystem::Update)
    auto collView = m_Scene->GetRegistry().view<TransformComponent, CollisionComponent>();
    for (auto &&[entityA, transformA, collisionA] : collView.each())
    {
        collisionA.hasCollision = false;
        collisionA.collidedWith = entt::null;

        for (auto &&[entityB, transformB, collisionB] : collView.each())
        {
            if (entityA == entityB)
                continue;
            if (!(collisionA.collisionMask & (1 << collisionB.collisionLayer)))
                continue;

            BoundingBox bA = collisionA.bounds;
            bA.min = Vector3Add(bA.min, transformA.position);
            bA.max = Vector3Add(bA.max, transformA.position);

            BoundingBox bB = collisionB.bounds;
            bB.min = Vector3Add(bB.min, transformB.position);
            bB.max = Vector3Add(bB.max, transformB.position);

            if (CheckCollisionBoxes(bA, bB))
            {
                collisionA.hasCollision = true;
                collisionA.collidedWith = entityB;
                collisionB.hasCollision = true;
                collisionB.collidedWith = entityA;
            }
        }
    }

    // 3. UPDATE LIFETIME (Previously LifetimeSystem::Update)
    auto lifetimeView = m_Scene->GetRegistry().view<LifetimeComponent>();
    std::vector<entt::entity> toDestroy;

    for (auto &&[entity, lifetime] : lifetimeView.each())
    {
        lifetime.timer += deltaTime;
        if (lifetime.timer >= lifetime.lifetime && lifetime.destroyOnTimeout)
            toDestroy.push_back(entity);
    }

    for (auto entity : toDestroy)
        m_Scene->GetRegistry().destroy(entity);

    // Update UI Overlay Scene
    if (SceneManager::IsInitialized())
    {
        auto uiScene = SceneManager::GetUIScene();
        if (uiScene && uiScene.get() != m_Scene.get() && ScriptManager::IsInitialized())
        {
            ScriptManager::SetActiveRegistry(&uiScene->GetRegistry());
            ScriptManager::UpdateScripts(uiScene->GetRegistry(), deltaTime);
        }
    }

    // 4. Handle Input Polling (Respawn)
    if (::IsKeyPressed(KEY_F))
    {
        auto view =
            m_Scene->GetRegistry().view<TransformComponent, VelocityComponent, PlayerComponent>();
        for (auto &&[entity, transform, velocity, player] : view.each())
        {
            transform.position = player.spawnPosition;
            velocity.velocity = {0, 0, 0};
            player.isGrounded = false;
            player.runTimer = 0;
            player.maxHeight = 0;
            player.cameraDistance = 10.0f;
            player.cameraPitch = 25.0f;
            player.cameraYaw = 0.0f;
            if (player.isFallingSoundPlaying)
            {
                Audio::StopLoopingSoundEffect("player_fall");
                player.isFallingSoundPlaying = false;
            }
            CD_INFO("[RuntimeLayer] Player respawned at (%.2f, %.2f, %.2f)", player.spawnPosition.x,
                    player.spawnPosition.y, player.spawnPosition.z);
        }
    }
}

void RuntimeLayer::OnRender()
{
    Renderer::BeginMode3D(Renderer::GetCamera());

    // Render Skybox from LevelManager's GameScene
    if (LevelManager::IsInitialized())
    {
        auto &gameScene = LevelManager::GetGameScene();
        if (gameScene.GetSkyBox() && gameScene.GetSkyBox()->IsLoaded())
        {
            gameScene.GetSkyBox()->DrawSkybox(Renderer::GetCamera().position);
        }
    }

    RenderScene();
    Renderer::EndMode3D();

    // Render 2D UI / HUD
    RenderUI((float)GetScreenWidth(), (float)GetScreenHeight());
}

void RuntimeLayer::RenderUI(float width, float height)
{
    // 1. Render World Scene UI
    UIRenderSystem::Render(m_Scene->GetRegistry(), (int)width, (int)height);
    UIRenderSystem::RenderImGui(m_Scene->GetRegistry(), (int)width, (int)height);

    // 2. Render UI Overlay Scene (if any)
    if (SceneManager::IsInitialized())
    {
        auto uiScene = SceneManager::GetUIScene();
        if (uiScene && uiScene.get() != m_Scene.get())
        {
            UIRenderSystem::Render(uiScene->GetRegistry(), (int)width, (int)height);
            UIRenderSystem::RenderImGui(uiScene->GetRegistry(), (int)width, (int)height);
        }
    }

    // 4. HUD SYSTEM LOGIC
    auto view = m_Scene->GetRegistry().view<PlayerComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto &playerComp = m_Scene->GetRegistry().get<PlayerComponent>(entity);
        auto &transform = m_Scene->GetRegistry().get<TransformComponent>(entity);

        int hours = (int)playerComp.runTimer / 3600;
        int minutes = ((int)playerComp.runTimer % 3600) / 60;
        int seconds = (int)playerComp.runTimer % 60;

        // UI Layout Constants
        const float margin = 30.0f;
        const float fontSize = 24.0f;
        const float spacing = 1.0f;
        const Color accentColor = WHITE;
        const Vector2 shadowOffset = {1.5f, 1.5f};
        const Color shadowColor = ColorAlpha(BLACK, 0.4f);

        // --- 1. TOP-LEFT HUD (Height & Timer) ---

        // Height Text
        std::string heightStr = std::to_string((int)playerComp.maxHeight) + "m";
        const char *heightText = heightStr.c_str();
        Vector2 heightTextSize =
            m_fontLoaded ? MeasureTextEx(m_hudFont, heightText, fontSize, spacing)
                         : Vector2{(float)MeasureText(heightText, (int)fontSize), fontSize};

        // Draw Height
        if (m_fontLoaded)
        {
            DrawTextEx(m_hudFont, heightText, {margin + shadowOffset.x, margin + shadowOffset.y},
                       fontSize, spacing, shadowColor);
            DrawTextEx(m_hudFont, heightText, {margin, margin}, fontSize, spacing, accentColor);
        }
        else
        {
            DrawText(heightText, (int)margin + 2, (int)margin + 2, (int)fontSize, BLACK);
            DrawText(heightText, (int)margin, (int)margin, (int)fontSize, WHITE);
        }

        // Timer Icon & Text
        float timerX = margin + heightTextSize.x + 25.0f;

        // Clock Icon (Minimalist)
        DrawCircleLines((int)timerX + 8, (int)margin + 12, 7, WHITE);
        DrawLine((int)timerX + 8, (int)margin + 12, (int)timerX + 8, (int)margin + 8, WHITE);
        DrawLine((int)timerX + 8, (int)margin + 12, (int)timerX + 11, (int)margin + 12, WHITE);

        const char *timerText = (hours > 0) ? TextFormat("%dh %dm %ds", hours, minutes, seconds)
                                            : TextFormat("%dm %ds", minutes, seconds);

        if (m_fontLoaded)
        {
            DrawTextEx(m_hudFont, timerText,
                       {timerX + 22.0f + shadowOffset.x, margin + shadowOffset.y}, fontSize,
                       spacing, shadowColor);
            DrawTextEx(m_hudFont, timerText, {timerX + 22.0f, margin}, fontSize, spacing,
                       accentColor);
        }
        else
        {
            DrawText(timerText, (int)timerX + 24, (int)margin + 2, (int)fontSize, BLACK);
            DrawText(timerText, (int)timerX + 22, (int)margin, (int)fontSize, WHITE);
        }

        // --- 2. VERTICAL HEIGHT METER (Left) ---
        float meterX = margin + 5.0f;
        float meterY = margin + 45.0f;
        float meterHeight = 120.0f;
        float meterWidth = 2.0f;

        // Draw main line
        DrawRectangle((int)meterX, (int)meterY, (int)meterWidth, (int)meterHeight,
                      ColorAlpha(WHITE, 0.5f));

        // Draw notches
        for (int i = 0; i <= 4; i++)
        {
            float notchY = meterY + (meterHeight * (i / 4.0f));
            DrawRectangle((int)meterX, (int)notchY, 6, 1, ColorAlpha(WHITE, 0.5f));
        }

        // Draw marker (Current vs Max relative position - simplified for now)
        // In "Only Up" it often shows relative progress or just a sleek marker
        float markerPos = meterY + meterHeight * 0.2f; // Dummy position for aesthetic
        DrawTriangle({meterX + 8, markerPos - 4}, {meterX + 8, markerPos + 4},
                     {meterX + 4, markerPos}, WHITE);

        // --- 3. INPUT TIPS (Bottom-Left) ---
        const char *tipText = "[F] Respawn";
        float tipFontSize = 18.0f;
        if (m_fontLoaded)
        {
            DrawTextEx(m_hudFont, tipText,
                       {margin + shadowOffset.x, height - margin - tipFontSize + shadowOffset.y},
                       tipFontSize, spacing, shadowColor);
            DrawTextEx(m_hudFont, tipText, {margin, height - margin - tipFontSize}, tipFontSize,
                       spacing, ColorAlpha(WHITE, 0.7f));
        }
        else
        {
            DrawText(tipText, (int)margin, (int)(height - margin - 20), 20,
                     ColorAlpha(WHITE, 0.6f));
        }
    }
}

void RuntimeLayer::RenderScene()
{
    // RENDER SYSTEM LOGIC (Previously RenderSystem::Render)
    auto view = m_Scene->GetRegistry().view<TransformComponent, RenderComponent>();
    std::vector<entt::entity> entities(view.begin(), view.end());

    // Sort by render layer
    std::sort(entities.begin(), entities.end(),
              [&](entt::entity a, entt::entity b)
              {
                  return view.get<RenderComponent>(a).renderLayer <
                         view.get<RenderComponent>(b).renderLayer;
              });

    for (auto entity : entities)
    {
        auto &t = view.get<TransformComponent>(entity);
        auto &r = view.get<RenderComponent>(entity);

        if (!r.visible || !r.model)
            continue;

        Matrix matS = MatrixScale(t.scale.x, t.scale.y, t.scale.z);
        Matrix matR = MatrixRotateXYZ(t.rotation);
        Matrix matT = MatrixTranslate(t.position.x + r.offset.x, t.position.y + r.offset.y,
                                      t.position.z + r.offset.z);

        r.model->transform = MatrixMultiply(MatrixMultiply(matS, matR), matT);

        // Check if editor is available for wireframe (Optional)
        // For now, simple render
        DrawModel(*r.model, Vector3Zero(), 1.0f, r.tint);
    }

    // DEBUG COLLISION RENDER
    if (true) // TODO: Implement CollisionDebugVisibility in Engine or Window
    {
        // TODO: Physics::RenderDebug() API has been removed
        // Physics::RenderDebug();

        auto playerView =
            m_Scene->GetRegistry().view<TransformComponent, CollisionComponent, PlayerComponent>();
        for (auto &&[entity, transform, collision, player] : playerView.each())
        {
            BoundingBox b = collision.bounds;
            b.min = Vector3Add(b.min, transform.position);
            b.max = Vector3Add(b.max, transform.position);
            DrawBoundingBox(b, RED);

            Vector3 rayStart = transform.position;
            rayStart.y += 1.0f;
            Vector3 rayEnd = rayStart;
            rayEnd.y -= 1.2f;
            DrawLine3D(rayStart, rayEnd, YELLOW);
        }
    }
}

void RuntimeLayer::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &event)
        {
            if (event.GetKeyCode() == KEY_F) // Respawn
            {
                auto view = m_Scene->GetRegistry()
                                .view<TransformComponent, VelocityComponent, PlayerComponent>();
                for (auto &&[entity, transform, velocity, player] : view.each())
                {
                    transform.position = player.spawnPosition;
                    velocity.velocity = {0, 0, 0};
                    player.isGrounded = false;
                    player.runTimer = 0;
                    player.maxHeight = 0;
                    player.cameraDistance = 10.0f;
                    player.cameraPitch = 25.0f;
                    player.cameraYaw = 0.0f;
                    if (player.isFallingSoundPlaying)
                    {
                        Audio::StopLoopingSoundEffect("player_fall");
                        player.isFallingSoundPlaying = false;
                    }
                }
                return true;
            }
            return false;
        });
}

} // namespace CHD
