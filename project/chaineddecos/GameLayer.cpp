#include "GameLayer.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/audio/Audio.h"
#include "core/input/Input.h"
#include "core/physics/Physics.h"
#include "core/renderer/Renderer.h"
#include "core/scripting/ScriptManager.h"
#include "editor/logic/ISceneManager.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/UIEventRegistry.h"
#include "logic/GameInitializer.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UtilityComponents.h"
#include "scene/ecs/components/VelocityComponent.h"
#include "scene/ecs/components/playerComponent.h"
#include "scene/ecs/systems/UIRenderSystem.h"
#include <algorithm>
#include <raylib.h>
#include <raymath.h>
#include <vector>

using namespace CHEngine;

namespace CHD
{

GameLayer::GameLayer() : Layer("GameLayer")
{
}

void GameLayer::OnAttach()
{
    // Register UI Events
    Engine::Instance().GetUIEventRegistry().Register(
        "start_game",
        []()
        {
            CD_INFO("[GameLayer] Start Game Event Triggered!");
            // Example logic: Reset player position
            auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();
            for (auto [entity, transform, velocity, player] : view.each())
            {
                transform.position = player.spawnPosition;
                velocity.velocity = {0, 0, 0};
            }
        });

    Engine::Instance().GetUIEventRegistry().Register(
        "quit_game",
        []()
        {
            CD_INFO("[GameLayer] Quit Game Event Triggered!");
            Engine::Instance().RequestExit();
        });

    CD_INFO("GameLayer Attached");

    // Load HUD Font via Initializer
    m_hudFont = CHD::GameInitializer::LoadHUDFont(m_fontLoaded);

    // Load Player Shader via Initializer
    int locWindDir;
    m_playerShader = CHD::GameInitializer::LoadPlayerShader(m_locFallSpeed, m_locTime, locWindDir);
    m_shaderLoaded = (m_playerShader.id != 0);

    // Initialize Scripts
    Engine::Instance().GetScriptManager().InitializeScripts();
}

void GameLayer::OnDetach()
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

    CD_INFO("GameLayer Detached");
}

void GameLayer::OnUpdate(float deltaTime)
{
    // Update Player Shader Uniforms
    if (m_shaderLoaded)
    {
        float time = (float)GetTime();
        SetShaderValue(m_playerShader, m_locTime, &time, SHADER_UNIFORM_FLOAT);

        auto playerView = REGISTRY.view<PlayerComponent, VelocityComponent>();
        for (auto &&[entity, player, velocity] : playerView.each())
        {
            float fallSpeed = (velocity.velocity.y < 0) ? std::abs(velocity.velocity.y) : 0.0f;
            SetShaderValue(m_playerShader, m_locFallSpeed, &fallSpeed, SHADER_UNIFORM_FLOAT);
        }
    }

    // UPDATE LUA SCRIPTS (Hazel style)
    Engine::Instance().GetScriptManager().UpdateScripts(deltaTime);

    // Sync ECS Transforms back to MapObjects for rendering consistency
    // Only applicable when running in editor context (ISceneManager available)
    static bool sceneManagerChecked = false;
    static std::shared_ptr<ISceneManager> cachedSceneManager = nullptr;
    if (!sceneManagerChecked)
    {
        cachedSceneManager = Engine::Instance().GetService<ISceneManager>();
        sceneManagerChecked = true;
    }
    if (cachedSceneManager)
    {
        cachedSceneManager->SyncEntitiesToMap();
    }

    // 1. UPDATE PLAYER LOGIC (Previously PlayerSystem::Update)
    auto playerView = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();

    for (auto [entity, transform, velocity, player] : playerView.each())
    {

        // Update Stats
        player.runTimer += deltaTime;
        if (transform.position.y > player.maxHeight)
            player.maxHeight = transform.position.y;

        // Handle Movement
        Vector2 mouseDelta = Input::GetMouseDelta();
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
        if (REGISTRY.all_of<PhysicsData>(entity))
        {
            auto &physics = REGISTRY.get<PhysicsData>(entity);
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
        if (REGISTRY.all_of<CollisionComponent>(entity))
        {
            auto &collision = REGISTRY.get<CollisionComponent>(entity);
            Vector3 center = proposedPos;
            center.x += (collision.bounds.max.x + collision.bounds.min.x) * 0.5f;
            center.y += (collision.bounds.max.y + collision.bounds.min.y) * 0.5f;
            center.z += (collision.bounds.max.z + collision.bounds.min.z) * 0.5f;

            Vector3 halfSize =
                Vector3Scale(Vector3Subtract(collision.bounds.max, collision.bounds.min), 0.5f);
            Collision playerCol(center, halfSize);
            Vector3 response = {0};

            if (Physics::CheckCollision(playerCol, response))
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

        // Camera Update
        ::Camera &camera = Renderer::GetCamera();
        float yawRadLocal = player.cameraYaw * DEG2RAD;
        float pitchRadLocal = player.cameraPitch * DEG2RAD;

        Vector3 camOffset = {player.cameraDistance * cosf(pitchRadLocal) * sinf(yawRadLocal),
                             player.cameraDistance * sinf(pitchRadLocal),
                             player.cameraDistance * cosf(pitchRadLocal) * cosf(yawRadLocal)};

        camera.target = Vector3Add(transform.position, {0, 1.5f, 0});
        camera.position = Vector3Add(camera.target, camOffset);
    }

    // 2. UPDATE ENTITY COLLISIONS (Previously EntityCollisionSystem::Update)
    auto collView = REGISTRY.view<TransformComponent, CollisionComponent>();
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
    auto lifetimeView = REGISTRY.view<LifetimeComponent>();
    std::vector<entt::entity> toDestroy;

    for (auto &&[entity, lifetime] : lifetimeView.each())
    {
        lifetime.timer += deltaTime;
        if (lifetime.timer >= lifetime.lifetime && lifetime.destroyOnTimeout)
            toDestroy.push_back(entity);
    }

    for (auto entity : toDestroy)
        REGISTRY.destroy(entity);
}

void GameLayer::OnRender()
{
    RenderScene();
}

void GameLayer::RenderUI(float width, float height)
{
    // Render stored UI elements
    UIRenderSystem::Render((int)width, (int)height);
    UIRenderSystem::RenderImGui((int)width, (int)height);

    // 4. HUD SYSTEM LOGIC (Previously HUDSystem::Render)
    auto view = REGISTRY.view<PlayerComponent>();

    for (auto entity : view)
    {
        auto &playerComp = view.get<PlayerComponent>(entity);

        int hours = (int)playerComp.runTimer / 3600;
        int minutes = ((int)playerComp.runTimer % 3600) / 60;
        int seconds = (int)playerComp.runTimer % 60;

        int startX = 40;
        int startY = 80;
        float spacing = 2.0f;

        // 1. Height Section
        const char *heightText = TextFormat("height : %.0fm", playerComp.maxHeight);
        float fontSizeHeight = 36.0f;
        Vector2 heightSize;

        if (m_fontLoaded)
            heightSize = MeasureTextEx(m_hudFont, heightText, fontSizeHeight, spacing);
        else
            heightSize = {(float)MeasureText(heightText, (int)fontSizeHeight), fontSizeHeight};

        // Draw Height Shadow
        Vector2 heightPos = {(float)startX, (float)startY};
        Vector2 shadowOffset = {2.0f, 2.0f};

        if (m_fontLoaded)
            DrawTextEx(m_hudFont, heightText,
                       {heightPos.x + shadowOffset.x, heightPos.y + shadowOffset.y}, fontSizeHeight,
                       spacing, ColorAlpha(BLACK, 0.5f));
        else
            DrawText(heightText, (int)(heightPos.x + shadowOffset.x),
                     (int)(heightPos.y + shadowOffset.y), (int)fontSizeHeight, BLACK);

        // Draw Height Text
        if (m_fontLoaded)
            DrawTextEx(m_hudFont, heightText, heightPos, fontSizeHeight, spacing, WHITE);
        else
            DrawText(heightText, (int)heightPos.x, (int)heightPos.y, (int)fontSizeHeight, WHITE);

        // Vertical separator bar
        int barX = (int)(heightPos.x + heightSize.x + 10);
        DrawLineEx({(float)barX, (float)startY}, {(float)barX, (float)startY + 30}, 3.0f, WHITE);

        // 2. Timer Section with Clock Icon
        const char *timerText;
        if (hours > 0)
            timerText = TextFormat("%02d:%02d:%02d", hours, minutes, seconds);
        else
            timerText = TextFormat("%02d:%02d", minutes, seconds);

        // Clock icon position
        int iconX = (int)(heightPos.x + heightSize.x + 20);
        int iconY = (int)(startY + 12);
        int iconRadius = 10;

        // Draw clock circle with shadow
        DrawCircle(iconX + 1, iconY + 1, (float)iconRadius, ColorAlpha(BLACK, 0.3f));
        DrawCircle(iconX, iconY, (float)iconRadius, WHITE);
        DrawCircle(iconX, iconY, (float)iconRadius - 1, ColorAlpha(SKYBLUE, 0.2f));

        // Clock hands
        DrawLine(iconX, iconY, iconX, iconY - 6, BLACK); // Hour hand
        DrawLine(iconX, iconY, iconX + 5, iconY, BLACK); // Minute hand
        DrawCircle(iconX, iconY, 2.0f, BLACK);           // Center dot

        // Timer text position
        int timerX = iconX + iconRadius + 8;
        float fontSizeTimer = 28.0f;

        // Draw Timer Shadow
        if (m_fontLoaded)
            DrawTextEx(m_hudFont, timerText,
                       {(float)timerX + shadowOffset.x, (float)startY + shadowOffset.y},
                       fontSizeTimer, spacing, ColorAlpha(BLACK, 0.5f));
        else
            DrawText(timerText, timerX + 2, startY + 2, (int)fontSizeTimer, BLACK);

        // Draw Timer Text
        if (m_fontLoaded)
            DrawTextEx(m_hudFont, timerText, {(float)timerX, (float)startY}, fontSizeTimer, spacing,
                       WHITE);
        else
            DrawText(timerText, timerX, startY, (int)fontSizeTimer, WHITE);
    }
}

void GameLayer::RenderScene()
{
    // RENDER SYSTEM LOGIC (Previously RenderSystem::Render)
    auto view = REGISTRY.view<TransformComponent, RenderComponent>();
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
    if (Engine::Instance().IsCollisionDebugVisible())
    {
        Physics::Render(); // Wait, I need to check if Physics has Render()

        auto playerView = REGISTRY.view<TransformComponent, CollisionComponent, PlayerComponent>();
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

void GameLayer::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &event)
        {
            if (event.GetKeyCode() == KEY_F) // Respawn
            {
                auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();
                for (auto &&[entity, transform, velocity, player] : view.each())
                {
                    transform.position = player.spawnPosition;
                    velocity.velocity = {0, 0, 0};
                    player.isGrounded = false;
                    player.runTimer = 0;
                    player.maxHeight = 0;
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
