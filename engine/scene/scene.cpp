#include "scene.h"
#include "components.h"
#include "engine/core/events.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "entity.h"
#include <cfloat>

namespace CHEngine
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

static bool CheckWallCollision(entt::registry &registry, entt::entity playerEntity,
                               const Vector3 &targetTranslation, const Vector3 &movementDir)
{
    auto &playerTransform = registry.get<TransformComponent>(playerEntity);
    Vector3 playerScale = playerTransform.Scale;

    // Default player AABB
    Vector3 playerMin =
        Vector3Subtract(targetTranslation, Vector3Multiply({0.4f, 0.0f, 0.4f}, playerScale));
    Vector3 playerMax =
        Vector3Add(targetTranslation, Vector3Multiply({0.4f, 1.8f, 0.4f}, playerScale));

    if (registry.all_of<ColliderComponent>(playerEntity))
    {
        auto &pc = registry.get<ColliderComponent>(playerEntity);
        playerMin = Vector3Add(targetTranslation, Vector3Multiply(pc.Offset, playerScale));
        playerMax = Vector3Add(playerMin, Vector3Multiply(pc.Size, playerScale));
    }

    auto colView = registry.view<TransformComponent, ColliderComponent>();
    for (auto other : colView)
    {
        if (other == playerEntity)
            continue;

        auto &otherTransform = colView.get<TransformComponent>(other);
        auto &otherCollider = colView.get<ColliderComponent>(other);

        if (!otherCollider.bEnabled)
            continue;

        if (otherCollider.Type == ColliderType::Box)
        {
            Vector3 otherScale = otherTransform.Scale;
            Vector3 otherMin = Vector3Add(otherTransform.Translation,
                                          Vector3Multiply(otherCollider.Offset, otherScale));
            Vector3 otherMax =
                Vector3Add(otherMin, Vector3Multiply(otherCollider.Size, otherScale));

            if (Physics::CheckAABB(playerMin, playerMax, otherMin, otherMax))
            {
                if (playerMin.y + 0.3f > otherMax.y)
                    continue; // Step up
                if (playerMax.y - 0.3f < otherMin.y)
                    continue; // High above

                otherCollider.IsColliding = true;
                return true;
            }
        }
        else if (otherCollider.Type == ColliderType::Mesh && otherCollider.BVHRoot)
        {
            float heights[2] = {0.3f, 1.0f};
            for (float h : heights)
            {
                Ray moveRay;
                moveRay.position = playerTransform.Translation;
                moveRay.position.y += h;
                moveRay.direction = movementDir;

                Matrix modelTransform = otherTransform.GetTransform();
                Matrix invTransform = MatrixInvert(modelTransform);
                Vector3 localOrigin = Vector3Transform(moveRay.position, invTransform);
                Vector3 localTarget =
                    Vector3Transform(Vector3Add(moveRay.position, moveRay.direction), invTransform);
                Vector3 localDir = Vector3Normalize(Vector3Subtract(localTarget, localOrigin));

                Ray localRay = {localOrigin, localDir};
                float t_local = FLT_MAX;
                Vector3 localNormal;

                if (BVHBuilder::Raycast(otherCollider.BVHRoot.get(), localRay, t_local,
                                        localNormal))
                {
                    if (t_local < 0.5f)
                    {
                        otherCollider.IsColliding = true;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    auto playerView = m_Registry.view<PlayerComponent, TransformComponent, RigidBodyComponent>();
    for (auto entity : playerView)
    {
        auto &player = playerView.get<PlayerComponent>(entity);
        auto &transform = playerView.get<TransformComponent>(entity);
        auto &rigidBody = playerView.get<RigidBodyComponent>(entity);

        float currentSpeed = player.MovementSpeed;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT))
            currentSpeed *= 2.0f;

        // Camera Rotation
        while (player.CameraYaw < 0)
            player.CameraYaw += 360.0f;
        while (player.CameraYaw >= 360.0f)
            player.CameraYaw -= 360.0f;

        float yawRadians = player.CameraYaw * DEG2RAD;
        Vector3 forwardDir = {-sinf(yawRadians), 0.0f, -cosf(yawRadians)};
        Vector3 rightDir = {cosf(yawRadians), 0.0f, -sinf(yawRadians)};

        // Movement Input (Polling for smooth movement)
        Vector3 movementVector = {0.0f, 0.0f, 0.0f};
        if (Input::IsKeyDown(KEY_W))
            movementVector = Vector3Add(movementVector, forwardDir);
        if (Input::IsKeyDown(KEY_S))
            movementVector = Vector3Subtract(movementVector, forwardDir);
        if (Input::IsKeyDown(KEY_A))
            movementVector = Vector3Subtract(movementVector, rightDir);
        if (Input::IsKeyDown(KEY_D))
            movementVector = Vector3Add(movementVector, rightDir);

        float magSq = movementVector.x * movementVector.x + movementVector.z * movementVector.z;
        if (magSq > 0.01f)
        {
            float mag = sqrtf(magSq);
            movementVector.x /= mag;
            movementVector.z /= mag;

            Vector3 targetPosition = {
                transform.Translation.x + movementVector.x * currentSpeed * deltaTime,
                transform.Translation.y,
                transform.Translation.z + movementVector.z * currentSpeed * deltaTime};

            if (!CheckWallCollision(m_Registry, entity, targetPosition, movementVector))
            {
                transform.Translation.x = targetPosition.x;
                transform.Translation.z = targetPosition.z;
            }

            transform.Rotation.y = atan2f(movementVector.x, movementVector.z);
        }

        // Jump logic moved to OnEvent for precise one-time trigger
    }
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // TODO: Editor specific updates
}

void Scene::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);

    // Jump on Space key press (event-driven for precise one-time trigger)
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (ev.GetKeyCode() == KEY_SPACE)
            {
                auto playerView = m_Registry.view<PlayerComponent, RigidBodyComponent>();
                for (auto entity : playerView)
                {
                    auto &player = playerView.get<PlayerComponent>(entity);
                    auto &rigidBody = playerView.get<RigidBodyComponent>(entity);

                    if (rigidBody.IsGrounded)
                    {
                        rigidBody.Velocity.y = player.JumpForce;
                        rigidBody.IsGrounded = false;
                        return true; // Event handled
                    }
                }
            }
            return false;
        });
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (ev.GetKeyCode() == KEY_F)
            {
                // Find active spawn point
                Vector3 spawnPoint = {0.0f, 0.0f, 0.0f};
                bool foundSpawn = false;

                auto spawnView = m_Registry.view<SpawnComponent>();
                for (auto spawnEntity : spawnView)
                {
                    auto &spawn = spawnView.get<SpawnComponent>(spawnEntity);
                    if (spawn.IsActive)
                    {
                        spawnPoint = spawn.SpawnPoint;
                        foundSpawn = true;
                        break;
                    }
                }

                if (!foundSpawn)
                    return false;

                // Teleport player to spawn point
                auto playerView = m_Registry.view<PlayerComponent, TransformComponent>();
                for (auto playerEntity : playerView)
                {
                    auto &transform = playerView.get<TransformComponent>(playerEntity);
                    transform.Translation = spawnPoint;
                    return true; // Event handled
                }
            }
            return false;
        });
}
} // namespace CHEngine
