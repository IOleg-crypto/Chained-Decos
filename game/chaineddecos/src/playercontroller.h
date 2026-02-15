#ifndef CH_PLAYER_CONTROLLER_H
#define CH_PLAYER_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "raymath.h"
#include "engine/core/application.h"
#include <cmath>

namespace CHEngine
{
    CH_SCRIPT(PlayerController)
    {
    public:
        CH_UPDATE(deltaTime)
        {
            auto scene = GetScene();
            if (!scene) return;

            bool hasPlayer = HasComponent<PlayerComponent>();
            bool hasRigidBody = HasComponent<RigidBodyComponent>();
            
            if (!hasPlayer)
                return;

            float currentSpeed = 15.0f;
            float jumpForce = 10.0f;
            
            if (hasPlayer)
            {
                auto &player = GetComponent<PlayerComponent>();
                currentSpeed = player.MovementSpeed;
                jumpForce = player.JumpForce;
            }
            auto &rigidBody = GetComponent<RigidBodyComponent>();

            bool wDown = Input::IsKeyDown(KEY_W);
            bool sDown = Input::IsKeyDown(KEY_S);
            bool aDown = Input::IsKeyDown(KEY_A);
            bool dDown = Input::IsKeyDown(KEY_D);

            if (Input::IsKeyDown(KEY_LEFT_SHIFT))
                currentSpeed *= 2.0f;

            // 1. Get orientation from the active camera (Hazel Style)
            auto sceneCamera = scene->GetActiveCamera();
            if (!sceneCamera.has_value())
                return;

            Camera3D camera = sceneCamera.value();
            Vector3 cameraForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 cameraRight = Vector3Normalize(Vector3CrossProduct(cameraForward, camera.up));
            
            // 2. Flatten for ground movement (XZ Plane)
            Vector3 forward = { cameraForward.x, 0.0f, cameraForward.z };
            Vector3 right = { cameraRight.x, 0.0f, cameraRight.z };
            
            if (Vector3LengthSqr(forward) > 0.0001f) forward = Vector3Normalize(forward);
            if (Vector3LengthSqr(right) > 0.0001f) right = Vector3Normalize(right);

            Vector3 movementVector = {0.0f, 0.0f, 0.0f};
            if (wDown) movementVector = Vector3Add(movementVector, forward);
            if (sDown) movementVector = Vector3Subtract(movementVector, forward);
            if (aDown) movementVector = Vector3Subtract(movementVector, right);
            if (dDown) movementVector = Vector3Add(movementVector, right);

            if (Vector3LengthSqr(movementVector) > 0.0001f)
            {
                movementVector = Vector3Normalize(movementVector);

                rigidBody.Velocity.x = movementVector.x * currentSpeed;
                rigidBody.Velocity.z = movementVector.z * currentSpeed;

                auto &transform = GetComponent<TransformComponent>();
                // atan2f(x, z) gives angle in radians. Convert to degrees.
                float targetYaw = atan2f(movementVector.x, movementVector.z) * RAD2DEG;
                transform.Rotation.y = targetYaw;
                
                // Sync Quaternion
                transform.RotationQuat = QuaternionFromEuler(
                    transform.Rotation.x * DEG2RAD,
                    transform.Rotation.y * DEG2RAD,
                    transform.Rotation.z * DEG2RAD
                );
            }
            else
            {
                rigidBody.Velocity.x = 0;
                rigidBody.Velocity.z = 0;
            }


            // Jump handling
            if (Input::IsKeyPressed(KEY_SPACE) && rigidBody.IsGrounded)
            {
                rigidBody.Velocity.y = jumpForce;
                rigidBody.IsGrounded = false;
                CH_CORE_INFO("Jump triggered! Force: {}", jumpForce);
            }

            // Shader Uniform Sync
            if (HasComponent<ShaderComponent>())
            {
                auto& shaderComp = GetComponent<ShaderComponent>();
                
                // fallSpeed: use vertical velocity absolute value if falling
                float fallSpeedValue = rigidBody.Velocity.y < 0 ? -rigidBody.Velocity.y : 0.0f;
                shaderComp.SetFloat("fallSpeed", fallSpeedValue);
                
                // time: sync with engine time
                shaderComp.SetFloat("time", (float)GetTime());
                
                // windDirection: default or based on movement
                shaderComp.SetVec3("windDirection", {1.0f, 0.0f, 0.5f});
            }
        }

        CH_EVENT(e)
        {
            EventDispatcher dispatcher(e);

            // Handle Teleport to Spawn
            dispatcher.Dispatch<KeyPressedEvent>(
                [this](KeyPressedEvent &ev)
                {
                    if (ev.GetKeyCode() == KEY_T)
                    {
                        auto &sceneRegistry = GetScene()->GetRegistry();
                        auto spawnZoneView = sceneRegistry.view<SpawnComponent>();

                        for (auto spawnEntity : spawnZoneView)
                        {
                            auto &spawnZone = spawnZoneView.get<SpawnComponent>(spawnEntity);
                            if (spawnZone.IsActive)
                            {
                                GetComponent<TransformComponent>().Translation = spawnZone.SpawnPoint;
                                return true;
                            }
                        }
                    }
                    return false;
                });
        }
    };
} // namespace CHEngine

#endif // CH_PLAYER_CONTROLLER_H
