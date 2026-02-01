#ifndef CH_PLAYER_CONTROLLER_H
#define CH_PLAYER_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "raymath.h"

namespace CHEngine
{
    CH_SCRIPT(PlayerController)
    {
    public:
        CH_UPDATE(deltaTime)
        {
            auto &player = GetComponent<PlayerComponent>();
            auto &rigidBody = RigidBody();

            float currentSpeed = player.MovementSpeed;
            if (Input::IsKeyDown(KEY_LEFT_SHIFT))
                currentSpeed *= 2.0f;

            float yawRadians = player.CameraYaw * DEG2RAD;
            glm::vec3 forwardDir = {-sinf(yawRadians), 0.0f, -cosf(yawRadians)};
            glm::vec3 rightDir = {cosf(yawRadians), 0.0f, -sinf(yawRadians)};

            glm::vec3 movementVector = {0.0f, 0.0f, 0.0f};
            if (Input::IsKeyDown(KEY_W))
                movementVector += forwardDir;
            if (Input::IsKeyDown(KEY_S))
                movementVector -= forwardDir;
            if (Input::IsKeyDown(KEY_A))
                movementVector -= rightDir;
            if (Input::IsKeyDown(KEY_D))
                movementVector += rightDir;

            if (glm::length2(movementVector) > 0.0001f)
            {
                movementVector = glm::normalize(movementVector);

                Velocity().x = movementVector.x * currentSpeed;
                Velocity().z = movementVector.z * currentSpeed;

                Vector3 euler = Rotation();
                euler.y = atan2f(movementVector.x, movementVector.z);
                Transform().SetRotation(euler);
            }
            else
            {
                Velocity().x = 0;
                Velocity().z = 0;
            }
        } // namespace CHEngine

        CH_EVENT(e)
        {
            EventDispatcher dispatcher(e);

            // Handle Jump
            dispatcher.Dispatch<KeyPressedEvent>(
                [this](KeyPressedEvent &ev)
                {
                    if (Input::IsKeyDown(KEY_SPACE))
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
                    if (Input::IsKeyDown(KEY_T))
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
        }
    };
} // namespace CHEngine

#endif // CH_PLAYER_CONTROLLER_H
