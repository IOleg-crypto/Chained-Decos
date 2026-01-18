#ifndef CH_PLAYER_CONTROLLER_NEW_H
#define CH_PLAYER_CONTROLLER_NEW_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include <raymath.h>

namespace CHEngine
{
// Example: PlayerController using NEW Input Action System
// This demonstrates how to use InputManager instead of hardcoded keys
CH_SCRIPT(PlayerControllerNew){public : void OnCreate() override{}

                               CH_UPDATE(deltaTime){if (!HasComponent<PlayerComponent>() ||
                                                        !HasComponent<RigidBodyComponent>()) return;

auto &player = GetComponent<PlayerComponent>();
auto &rigidBody = RigidBody();

// 1. Poll Input Actions directly (KISS)
Vector2 movementInput = Input::GetActionAxis("Move");
bool isSprinting = Input::IsActionDown("Sprint");

if (Input::IsActionPressed("Jump"))
    HandleJump();
if (Input::IsActionPressed("Interact"))
    HandleInteract();
if (Input::IsActionPressed("Teleport"))
    HandleTeleport();

// 2. Calculate movement
float currentSpeed = player.MovementSpeed;
if (isSprinting)
    currentSpeed *= 2.0f;

float yawRadians = player.CameraYaw * DEG2RAD;
Vector3 forwardDir = {-sinf(yawRadians), 0.0f, -cosf(yawRadians)};
Vector3 rightDir = {cosf(yawRadians), 0.0f, -sinf(yawRadians)};

Vector3 movementVector = {movementInput.x, 0.0f, movementInput.y};

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
} // namespace CHEngine

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
}
;

} // namespace CHEngine

#endif // CH_PLAYER_CONTROLLER_NEW_H
