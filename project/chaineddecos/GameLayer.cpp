#include "GameLayer.h"
#include "components/audio/core/AudioManager.h"
#include "components/input/core/InputManager.h"
#include "components/physics/collision/core/collisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/Engine.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UtilityComponents.h"
#include "scene/ecs/components/VelocityComponent.h"
#include "scene/ecs/components/playerComponent.h"
#include "scene/ecs/systems/UIRenderSystem.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include "events/UIEventRegistry.h"
#include "editor/IEditor.h"
#include "scene/main/core/World.h"
#include "scene/resources/model/core/Model.h"
#include <algorithm>
#include <raylib.h>
#include <raymath.h>

using namespace ChainedEngine;
#include <vector>

using namespace ChainedDecos;

GameLayer::GameLayer() : Layer("GameLayer")
{
}

void GameLayer::OnAttach()
{
    // Register UI Events
    UIEventRegistry::Get().Register(
        "start_game",
        []()
        {
            TraceLog(LOG_INFO, "[GameLayer] Start Game Event Triggered!");
            // Example logic: Reset player position
            auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();
            for (auto [entity, transform, velocity, player] : view.each())
            {
                transform.position = player.spawnPosition;
                velocity.velocity = {0, 0, 0};
            }
        });

    UIEventRegistry::Get().Register("quit_game",
                                    []()
                                    {
                                        TraceLog(LOG_INFO,
                                                 "[GameLayer] Quit Game Event Triggered!");
                                        Engine::Instance().RequestExit();
                                    });

    TraceLog(LOG_INFO, "GameLayer Attached");

    // Load HUD Font
#ifdef PROJECT_ROOT_DIR
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/gantari/static/gantari-Bold.ttf";
    m_hudFont = LoadFontEx(fontPath.c_str(), 96, 0, 0);

    if (m_hudFont.baseSize > 0)
    {
        SetTextureFilter(m_hudFont.texture, TEXTURE_FILTER_BILINEAR);
        m_fontLoaded = true;
        TraceLog(LOG_INFO, "[GameLayer] Loaded HUD font: %s", fontPath.c_str());
    }
    else
    {
        TraceLog(LOG_WARNING, "[GameLayer] Failed to load HUD font: %s", fontPath.c_str());
        m_hudFont = GetFontDefault();
    }
#else
    m_hudFont = GetFontDefault();
#endif
}

void GameLayer::OnDetach()
{
    TraceLog(LOG_INFO, "GameLayer Detached");
}

void GameLayer::OnUpdate(float deltaTime)
{
    // 1. STATS & MOVEMENT (formerly PlayerSystem)
    auto playerView = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();
    auto collisionManager = Engine::Instance().GetService<CollisionManager>();

    for (auto [entity, transform, velocity, player] : playerView.each())
    {
        // Update Stats
        player.runTimer += deltaTime;
        if (transform.position.y > player.maxHeight)
            player.maxHeight = transform.position.y;

        // Handle Movement
        Vector3 moveDir = {0, 0, 0};
        float yawRad = player.cameraYaw * DEG2RAD;
        Vector3 forward = Vector3Normalize({-sinf(yawRad), 0.0f, -cosf(yawRad)});
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));

        auto &input = InputManager::Get();
        if (input.IsKeyDown(KEY_W))
            moveDir = Vector3Add(moveDir, forward);
        if (input.IsKeyDown(KEY_S))
            moveDir = Vector3Subtract(moveDir, forward);
        if (input.IsKeyDown(KEY_D))
            moveDir = Vector3Add(moveDir, right);
        if (input.IsKeyDown(KEY_A))
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
        if (input.IsKeyDown(KEY_LEFT_SHIFT) && player.isGrounded)
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

        // Jump handled in update for quick response
        if (input.IsKeyPressed(KEY_SPACE) && player.isGrounded)
        {
            velocity.velocity.y = player.jumpForce;
            player.isGrounded = false;
        }

        // 2. PHYSICS (formerly MovementSystem)
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

        if (collisionManager && REGISTRY.all_of<CollisionComponent>(entity))
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

            if (collisionManager->CheckCollision(playerCol, response))
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
            if (collisionManager->RaycastDown(rayOrigin, 1.2f, hitDist, hp, hn))
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

        // Camera Update
        Vector2 mouseDelta = input.GetMouseDelta();
        player.cameraDistance -= input.GetMouseWheelMove() * 1.5f;
        player.cameraDistance = Clamp(player.cameraDistance, 2.0f, 20.0f);
        player.cameraYaw -= mouseDelta.x * player.mouseSensitivity;
        player.cameraPitch =
            Clamp(player.cameraPitch - mouseDelta.y * player.mouseSensitivity, -85.0f, 85.0f);

        Camera &camera = RenderManager::Get().GetCamera();
        float yawRadLocal = player.cameraYaw * DEG2RAD;
        float pitchRadLocal = player.cameraPitch * DEG2RAD;
        Vector3 camOffset = {player.cameraDistance * cosf(pitchRadLocal) * sinf(yawRadLocal),
                             player.cameraDistance * sinf(pitchRadLocal),
                             player.cameraDistance * cosf(pitchRadLocal) * cosf(yawRadLocal)};
        camera.target = Vector3Add(transform.position, {0, 1.5f, 0});
        camera.position = Vector3Add(camera.target, camOffset);

        // Audio Update
        const float fallThresh = -5.0f;
        if (velocity.velocity.y < fallThresh && !player.isFallingSoundPlaying)
        {
            AudioManager::Get().PlayLoopingSoundEffect("player_fall", 1.0f);
            player.isFallingSoundPlaying = true;
        }
        else if ((velocity.velocity.y >= fallThresh || player.isGrounded) &&
                 player.isFallingSoundPlaying)
        {
            AudioManager::Get().StopLoopingSoundEffect("player_fall");
            player.isFallingSoundPlaying = false;
        }
    }

    // 3. LIFETIME (formerly LifetimeSystem)
    auto lifetimeView = REGISTRY.view<LifetimeComponent>();
    std::vector<entt::entity> toDestroy;
    for (auto [entity, lifetime] : lifetimeView.each())
    {
        lifetime.timer += deltaTime;
        if (lifetime.timer >= lifetime.lifetime && lifetime.destroyOnTimeout)
            toDestroy.push_back(entity);
    }
    for (auto entity : toDestroy)
        REGISTRY.destroy(entity);

    // 4. ENTITY-ENTITY COLLISION (formerly CollisionSystem)
    auto collView = REGISTRY.view<TransformComponent, CollisionComponent>();
    for (auto [entityA, transformA, collisionA] : collView.each())
    {
        collisionA.hasCollision = false;
        collisionA.collidedWith = entt::null;

        for (auto [entityB, transformB, collisionB] : collView.each())
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
}

void GameLayer::OnRender()
{
    RenderScene();
}

void GameLayer::RenderUI(float width, float height)
{
    // Render stored UI elements
    UIRenderSystem::Render((int)width, (int)height);

    auto view = REGISTRY.view<PlayerComponent>();
    for (auto entity : view)
    {
        auto &playerComp = view.get<PlayerComponent>(entity);

        int hours = (int)playerComp.runTimer / 3600;
        int minutes = ((int)playerComp.runTimer % 3600) / 60;
        int seconds = (int)playerComp.runTimer % 60;

        // Position HUD in top-left of viewport
        int startX = 20;
        int startY = 20;
        float spacing = 2.0f;

        // 1. Height Section
        const char *heightText = TextFormat("height : %.0fm", playerComp.maxHeight);
        float fontSizeHeight = 24.0f;

        if (m_fontLoaded)
            DrawTextEx(m_hudFont, heightText, {(float)startX, (float)startY}, fontSizeHeight,
                       spacing, WHITE);
        else
            DrawText(heightText, startX, startY, (int)fontSizeHeight, WHITE);

        // 2. Timer Section
        const char *timerText = TextFormat("timer : %02d:%02d:%02d", hours, minutes, seconds);
        float fontSizeTimer = 22.0f;

        if (m_fontLoaded)
            DrawTextEx(m_hudFont, timerText, {(float)startX, (float)startY + 30}, fontSizeTimer,
                       spacing, WHITE);
        else
            DrawText(timerText, startX, startY + 30, (int)fontSizeTimer, WHITE);
    }
}

void GameLayer::RenderScene()
{
    // MOVED FROM RenderSystem::Render
    auto view = REGISTRY.view<TransformComponent, RenderComponent>();
    std::vector<entt::entity> entities(view.begin(), view.end());

    std::sort(entities.begin(), entities.end(),
              [&](entt::entity a, entt::entity b)
              {
                  return view.get<RenderComponent>(a).renderLayer <
                         view.get<RenderComponent>(b).renderLayer;
              });

    for (auto entity : entities)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &renderComp = view.get<RenderComponent>(entity);

        if (!renderComp.visible || !renderComp.model)
            continue;

        Matrix matS = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);
        Matrix matR = MatrixRotateXYZ(transform.rotation);
        Matrix matT = MatrixTranslate(transform.position.x + renderComp.offset.x,
                                      transform.position.y + renderComp.offset.y,
                                      transform.position.z + renderComp.offset.z);

        renderComp.model->transform = MatrixMultiply(MatrixMultiply(matS, matR), matT);

        auto editor = Engine::Instance().GetService<IEditor>();
        bool wireframe = (editor && editor->IsWireframeEnabled());

        if (wireframe)
            DrawModelWires(*renderComp.model, Vector3Zero(), 1.0f, renderComp.tint);
        else
            DrawModel(*renderComp.model, Vector3Zero(), 1.0f, renderComp.tint);
    }

    // DEBUG COLLISION RENDER
    auto editor = Engine::Instance().GetService<IEditor>();
    if (editor && editor->IsCollisionDebugEnabled())
    {
        auto collisionManager = Engine::Instance().GetService<CollisionManager>();
        if (collisionManager)
        {
            collisionManager->Render();
        }

        // Draw player character's ACTIVE collision volume
        auto playerView = REGISTRY.view<TransformComponent, CollisionComponent, PlayerComponent>();
        for (auto [entity, transform, collision, player] : playerView.each())
        {
            // Bounding box from collision bounds + actual transform
            // This is the SAME box used for physics resolution in OnUpdate
            BoundingBox b = collision.bounds;
            b.min = Vector3Add(b.min, transform.position);
            b.max = Vector3Add(b.max, transform.position);
            DrawBoundingBox(b, RED);

            // Ground check ray visualization
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
                for (auto [entity, transform, velocity, player] : view.each())
                {
                    transform.position = player.spawnPosition;
                    velocity.velocity = {0, 0, 0};
                    player.isGrounded = false;
                    player.runTimer = 0;
                    player.maxHeight = 0;
                    if (player.isFallingSoundPlaying)
                    {
                        AudioManager::Get().StopLoopingSoundEffect("player_fall");
                        player.isFallingSoundPlaying = false;
                    }
                }
                return true;
            }
            return false;
        });
}
