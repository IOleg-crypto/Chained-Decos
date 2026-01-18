#ifndef CH_PLAYER_CONTROLLER_H
#define CH_PLAYER_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include <raymath.h>

namespace CHEngine
{
CH_SCRIPT(PlayerController){public : CH_UPDATE(deltaTime){
    if (!HasComponent<PlayerComponent>() || !HasComponent<RigidBodyComponent>()) return;

auto &player = GetComponent<PlayerComponent>();
auto &rigidBody = RigidBody();

float currentSpeed = player.MovementSpeed;
if (Input::IsKeyDown(KEY_LEFT_SHIFT))
    currentSpeed *= 2.0f;

float yawRadians = player.CameraYaw * DEG2RAD;
Vector3 forwardDir = {-sinf(yawRadians), 0.0f, -cosf(yawRadians)};
Vector3 rightDir = {cosf(yawRadians), 0.0f, -sinf(yawRadians)};

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

    Velocity().x = movementVector.x * currentSpeed;
    Velocity().z = movementVector.z * currentSpeed;

    Rotation().y = atan2f(movementVector.x, movementVector.z);
}
else
{
    Velocity().x = 0;
    Velocity().z = 0;
}
} // namespace CHEngine

CH_EVENT(e)
{
    if (!HasComponent<PlayerComponent>() || !HasComponent<RigidBodyComponent>())
        return;

    EventDispatcher dispatcher(e);

    // Handle Jump
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (Input::IsKeyPressed(KEY_SPACE))
            {
                auto &player = GetComponent<PlayerComponent>();
                auto &rb = RigidBody();

                if (rb.IsGrounded)
                {
                    rb.Velocity.y = player.JumpForce;
                    rb.IsGrounded = false;
                    return true;
                }
            }
            return false;
        });

    // Handle Teleport to Spawn
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (Input::IsKeyPressed(KEY_F))
            {
                auto &sceneRegistry = GetEntity().GetScene()->GetRegistry();
                auto spawnZoneView = sceneRegistry.view<SpawnComponent>();

                for (auto spawnEntity : spawnZoneView)
                {
                    auto &spawnZone = spawnZoneView.get<SpawnComponent>(spawnEntity);
                    if (spawnZone.IsActive)
                    {
                        Translation() = spawnZone.SpawnPoint;
                        return true;
                    }
                }
            }
            return false;
        });

    // Example: Trigger Animation on 'E' press
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (ev.GetKeyCode() == KEY_E)
            {
                if (HasComponent<AnimationComponent>())
                {
                    auto &animation = GetComponent<AnimationComponent>();
                    animation.Play(1); // Play second animation
                    CH_CORE_INFO("Player triggered animation!");
                    return true;
                }
            }
            return false;
        });
}
}
;
} // namespace CHEngine

#endif // CH_PLAYER_CONTROLLER_H
