#ifndef CH_PLAYER_CONTROLLER_NEW_H
#define CH_PLAYER_CONTROLLER_NEW_H

#include "engine/core/input.h"
#include "engine/core/input_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include <raymath.h>

namespace CHEngine
{
// Example: PlayerController using NEW Input Action System
// This demonstrates how to use InputManager instead of hardcoded keys
CH_SCRIPT(PlayerControllerNew)
{
public:
    void OnCreate() override
    {
        // Subscribe to Input Actions (defined in gameplay_input.json)
        InputManager::SubscribeToAction("Move", [this](Vector2 axis) { m_MovementInput = axis; });

        InputManager::SubscribeToAction("Sprint", [this]() { m_IsSprinting = true; });

        InputManager::SubscribeToAction("Jump", [this]() { HandleJump(); });

        InputManager::SubscribeToAction("Interact", [this]() { HandleInteract(); });

        InputManager::SubscribeToAction("Teleport", [this]() { HandleTeleport(); });
    }

    CH_UPDATE(deltaTime)
    {
        auto &player = GetComponent<PlayerComponent>();
        auto &rigidBody = RigidBody();

        // Calculate movement from accumulated input
        float currentSpeed = player.MovementSpeed;
        if (m_IsSprinting)
            currentSpeed *= 2.0f;

        float yawRadians = player.CameraYaw * DEG2RAD;
        Vector3 forwardDir = {-sinf(yawRadians), 0.0f, -cosf(yawRadians)};
        Vector3 rightDir = {cosf(yawRadians), 0.0f, -sinf(yawRadians)};

        // Use accumulated movement input from InputManager
        Vector3 movementVector = {0.0f, 0.0f, 0.0f};
        movementVector.x = m_MovementInput.x;
        movementVector.z = m_MovementInput.y;

        // Apply movement
        float magSq = movementVector.x * movementVector.x + movementVector.z * movementVector.z;
        if (magSq > 0.01f)
        {
            float mag = sqrtf(magSq);
            movementVector.x /= mag;
            movementVector.z /= mag;

            Vector3 worldMovement = Vector3Add(Vector3Scale(rightDir, movementVector.x),
                                               Vector3Scale(forwardDir, movementVector.z));

            Velocity().x = worldMovement.x * currentSpeed;
            Velocity().z = worldMovement.z * currentSpeed;

            Rotation().y = atan2f(worldMovement.x, worldMovement.z);
        }
        else
        {
            Velocity().x = 0;
            Velocity().z = 0;
        }

        // Reset sprint flag (will be set again next frame if key is held)
        m_IsSprinting = false;
    }

private:
    void HandleJump()
    {
        auto &player = GetComponent<PlayerComponent>();
        auto &rb = RigidBody();

        if (rb.IsGrounded)
        {
            rb.Velocity.y = player.JumpForce;
            rb.IsGrounded = false;
        }
    }

    void HandleInteract()
    {
        if (HasComponent<AnimationComponent>())
        {
            auto &animation = GetComponent<AnimationComponent>();
            animation.Play(1);
            CH_CORE_INFO("Player triggered animation!");
        }
    }

    void HandleTeleport()
    {
        auto &sceneRegistry = GetEntity().GetScene()->GetRegistry();
        auto spawnZoneView = sceneRegistry.view<SpawnComponent>();

        for (auto spawnEntity : spawnZoneView)
        {
            auto &spawnZone = spawnZoneView.get<SpawnComponent>(spawnEntity);
            if (spawnZone.IsActive)
            {
                Translation() = spawnZone.SpawnPoint;
                CH_CORE_INFO("Player teleported to spawn!");
                return;
            }
        }
    }

    // Input state
    Vector2 m_MovementInput = {0.0f, 0.0f};
    bool m_IsSprinting = false;
};

} // namespace CHEngine

#endif // CH_PLAYER_CONTROLLER_NEW_H
