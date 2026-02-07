#ifndef CH_PLAYER_CONTROLLER_H
#define CH_PLAYER_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
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
            auto scene = GetEntity().GetScene();
            if (!scene) return;

            static int frame = 0;
            bool hasPlayer = HasComponent<PlayerComponent>();
            bool hasRigidBody = HasComponent<RigidBodyComponent>();
            
            if (frame % 60 == 0) {
                CH_CORE_INFO("[DIAG] PlayerController Update - Frame: {}, HasPlayer: {}, HasRB: {}", frame, hasPlayer, hasRigidBody);
            }

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
            else if (frame % 300 == 0)
            {
                CH_CORE_WARN("PlayerController: Missing PlayerComponent on entity '{}'. Using default speed (15.0).", 
                    GetEntity().GetComponent<TagComponent>().Tag);
            }

            if (!hasRigidBody)
            {
                if (frame % 60 == 0) CH_CORE_WARN("PlayerController: Missing RigidBodyComponent on entity '{}'. Movement disabled.", 
                    GetEntity().GetComponent<TagComponent>().Tag);
                return;
            }
            auto &rigidBody = GetComponent<RigidBodyComponent>();

            bool wDown = Input::IsKeyDown(KEY_W);
            bool sDown = Input::IsKeyDown(KEY_S);
            bool aDown = Input::IsKeyDown(KEY_A);
            bool dDown = Input::IsKeyDown(KEY_D);

            if (Input::IsKeyDown(KEY_LEFT_SHIFT))
                currentSpeed *= 2.0f;

            // 1. Get orientation from the active camera (Hazel Style)
            Camera3D camera = scene->GetActiveCamera();
            Vector3 cameraForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 cameraRight = Vector3Normalize(Vector3CrossProduct(cameraForward, camera.up));
            
            // 2. Flatten for ground movement (XZ Plane)
            glm::vec3 forward = { cameraForward.x, 0.0f, cameraForward.z };
            glm::vec3 right = { cameraRight.x, 0.0f, cameraRight.z };
            
            if (glm::length2(forward) > 0.0001f) forward = glm::normalize(forward);
            if (glm::length2(right) > 0.0001f) right = glm::normalize(right);

            glm::vec3 movementVector = {0.0f, 0.0f, 0.0f};
            if (wDown) movementVector += forward;
            if (sDown) movementVector -= forward;
            if (aDown) movementVector -= right;
            if (dDown) movementVector += right;

            if (glm::length2(movementVector) > 0.0001f)
            {
                movementVector = glm::normalize(movementVector);

                rigidBody.Velocity.x = movementVector.x * currentSpeed;
                rigidBody.Velocity.z = movementVector.z * currentSpeed;

                auto &transform = GetComponent<TransformComponent>();
                // atan2f(x, z) gives angle in radians. Convert to degrees.
                // We want the player to look where they move.
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

            frame++;

            // Jump handling (polling is more reliable for gameplay controls)
            if (Input::IsKeyPressed(KEY_SPACE) && rigidBody.IsGrounded)
            {
                rigidBody.Velocity.y = jumpForce;
                rigidBody.IsGrounded = false;
                CH_CORE_INFO("Jump triggered! Force: {}", jumpForce);
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
                        auto &sceneRegistry = GetEntity().GetScene()->GetRegistry();
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
