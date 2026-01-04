#include "scene.h"
#include "components.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "entity.h"

namespace CH
{
Scene::Scene()
{
}

Entity Scene::CreateEntity(const std::string &name)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();

    CH_CORE_INFO("Entity Created: %s (%d)", name, (uint32_t)entity);
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    if (!m_Registry.valid(entity))
    {
        CH_CORE_WARN("Attempted to destroy invalid entity: %d", (uint32_t)entity);
        return;
    }
    CH_CORE_INFO("Entity Destroyed: %d", (uint32_t)entity);
    m_Registry.destroy(entity);
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    // 1. Player Spawning
    auto playerView = m_Registry.view<PlayerComponent>();
    if (playerView.begin() == playerView.end())
    {
        auto spawnView = m_Registry.view<SpawnComponent, TransformComponent>();
        if (spawnView.begin() != spawnView.end())
        {
            auto spawnEntity = *spawnView.begin();
            auto &spawnTransform = spawnView.get<TransformComponent>(spawnEntity);

            auto player = CreateEntity("Player");
            player.AddComponent<PlayerComponent>();
            player.AddComponent<RigidBodyComponent>();
            player.AddComponent<BoxColliderComponent>(); // Will be auto-calculated
            player.AddComponent<ModelComponent>(PROJECT_ROOT_DIR "/resources/player_low.glb");
            player.GetComponent<TransformComponent>().Translation = spawnTransform.Translation;
            CH_CORE_INFO("Player Spawned at: %.2f, %.2f, %.2f", spawnTransform.Translation.x,
                         spawnTransform.Translation.y, spawnTransform.Translation.z);
        }
    }

    // 2. Player Movement
    auto view = m_Registry.view<PlayerComponent, TransformComponent, RigidBodyComponent>();
    for (auto entity : view)
    {
        auto &player = view.get<PlayerComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);
        auto &rb = view.get<RigidBodyComponent>(entity);

        float speed = player.MovementSpeed;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT))
            speed *= 2.0f;

        Vector3 movement = {0.0f, 0.0f, 0.0f};

        float yawRad = player.CameraYaw * DEG2RAD;
        Vector3 forward = {sinf(yawRad), 0.0f, -cosf(yawRad)};
        Vector3 right = {cosf(yawRad), 0.0f, -sinf(yawRad)};

        if (Input::IsKeyDown(KEY_W))
            movement = Vector3Add(movement, forward);
        if (Input::IsKeyDown(KEY_S))
            movement = Vector3Subtract(movement, forward);
        if (Input::IsKeyDown(KEY_A))
            movement = Vector3Subtract(movement, right);
        if (Input::IsKeyDown(KEY_D))
            movement = Vector3Add(movement, right);

        if (Vector3Length(movement) > 0.1f)
        {
            movement = Vector3Normalize(movement);
            Vector3 targetTranslation =
                Vector3Add(transform.Translation, Vector3Scale(movement, speed * deltaTime));

            // Collision Resolution (AABB vs AABB)
            auto colliders = m_Registry.view<TransformComponent, BoxColliderComponent>();
            bool wallHit = false;

            Vector3 playerMin = Vector3Subtract(targetTranslation, {0.4f, 0.0f, 0.4f});
            Vector3 playerMax = Vector3Add(targetTranslation, {0.4f, 1.8f, 0.4f});

            if (m_Registry.all_of<BoxColliderComponent>(entity))
            {
                auto &pc = m_Registry.get<BoxColliderComponent>(entity);
                playerMin = Vector3Add(targetTranslation, pc.Offset);
                playerMax = Vector3Add(playerMin, pc.Size);
            }

            for (auto other : colliders)
            {
                if (other == entity)
                    continue;

                auto &ot = colliders.get<TransformComponent>(other);
                auto &oc = colliders.get<BoxColliderComponent>(other);

                Vector3 otherMin = Vector3Add(ot.Translation, oc.Offset);
                Vector3 otherMax = Vector3Add(otherMin, oc.Size);

                if (Physics::CheckAABB(playerMin, playerMax, otherMin, otherMax))
                {
                    wallHit = true;
                    break;
                }
            }

            if (!wallHit)
            {
                transform.Translation.x = targetTranslation.x;
                transform.Translation.z = targetTranslation.z;
            }

            float targetAngle = atan2f(movement.x, movement.z);
            transform.Rotation = {0.0f, targetAngle, 0.0f};
        }

        // Jump logic using RigidBody
        if (rb.IsGrounded && Input::IsKeyPressed(KEY_SPACE))
        {
            rb.Velocity.y = 10.0f; // Jump force
            rb.IsGrounded = false;
        }
    }
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // TODO: Editor specific updates
}
} // namespace CH
