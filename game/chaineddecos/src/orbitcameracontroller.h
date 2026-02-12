#ifndef CH_ORBIT_CAMERA_CONTROLLER_H
#define CH_ORBIT_CAMERA_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "raymath.h" // Added raymath for Clamp

namespace CHEngine
{
    CH_SCRIPT(OrbitCameraController)
    {
    public:
        CH_UPDATE(deltaTime)
        {
            auto scene = GetScene();
            if (!scene) return;

            // Get camera component
            if (!GetEntity().HasComponent<CameraComponent>())
            {
                ///CH_CORE_ERROR("OrbitCameraController: Entity does not have CameraComponent!");
                return;
            }

            auto &camera = GetComponent<CameraComponent>();
            if (!camera.IsOrbitCamera)
            {
                //CH_CORE_WARN("OrbitCameraController: IsOrbitCamera is false, skipping update");
                return;
            }

            // 1. Find the target entity (Player)
            Entity targetEntity = scene->FindEntityByTag(camera.TargetEntityTag);
            if (!targetEntity)
            {
                // Fallback: search for entity with PlayerComponent
                auto playerView = scene->GetRegistry().view<PlayerComponent>();
                if (playerView.begin() != playerView.end())
                    targetEntity = Entity(*playerView.begin(), &scene->GetRegistry());
            }

            if (!targetEntity || !targetEntity.HasComponent<TransformComponent>())
            {
                //CH_CORE_WARN("OrbitCameraController: Target entity '{}' not found!", camera.TargetEntityTag);
                return;
            }

            auto &targetTransform = targetEntity.GetComponent<TransformComponent>();

            // 2. Handle Input (Orbit & Zoom)
            bool isStandalone = !Application::Get().GetLayerStack().HasLayer("EditorLayer");
            if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || isStandalone)
            {
                Vector2 mouseDelta = Input::GetMouseDelta();
                camera.OrbitYaw -= mouseDelta.x * camera.LookSensitivity;
                camera.OrbitPitch -= mouseDelta.y * camera.LookSensitivity;

                // Clamp pitch to prevent flipping
                camera.OrbitPitch = Clamp(camera.OrbitPitch, -10.0f, 85.0f);
            }

            float wheelMovement = Input::GetMouseWheelMove();
            camera.OrbitDistance -= wheelMovement * 2.0f;
            camera.OrbitDistance = Clamp(camera.OrbitDistance, 2.0f, 40.0f);

            // 3. Calculate Camera Position (Spherical Coordinates)
            float yawRad = camera.OrbitYaw * DEG2RAD;
            float pitchRad = camera.OrbitPitch * DEG2RAD;

            // Standard orbit math
            float x = camera.OrbitDistance * cosf(pitchRad) * sinf(yawRad);
            float y = camera.OrbitDistance * sinf(pitchRad);
            float z = camera.OrbitDistance * cosf(pitchRad) * cosf(yawRad);

            Vector3 targetPos = targetTransform.Translation;
            targetPos.y += 1.5f; // Look at target's head/chest level

            // 4. Update Camera Transform
            auto &transform = GetComponent<TransformComponent>();
            
            if (camera.OrbitDistance < 0.1f)
            {
                // First Person: Position exactly at head, maybe offset forward a tiny bit
                transform.Translation = targetPos;
            }
            else
            {
                // Third Person: Orbit
                transform.Translation = Vector3Add(targetPos, {x, y, z});
            }

            // 5. Update Rotation (Look At Target)
            transform.Rotation.x = -camera.OrbitPitch;
            transform.Rotation.y = camera.OrbitYaw; 
            transform.Rotation.z = 0.0f;
            
            // Sync Quaternion
            transform.RotationQuat = QuaternionFromEuler(
                transform.Rotation.x * DEG2RAD,
                transform.Rotation.y * DEG2RAD,
                transform.Rotation.z * DEG2RAD
            );
        }
    };
} // namespace CHEngine

#endif // CH_ORBIT_CAMERA_CONTROLLER_H
