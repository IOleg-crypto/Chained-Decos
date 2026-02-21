#ifndef CH_CAMERA_CONTROLLER_H
#define CH_CAMERA_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "raymath.h"

namespace CHEngine
{
CH_SCRIPT(CameraController){public : CH_UPDATE(deltaTime){auto scene = GetScene();
if (!scene)
{
    return;
}

// 1. Get CameraComponent context
if (!GetEntity().HasComponent<CameraComponent>())
{
    return;
}
auto& camera = GetComponent<CameraComponent>();

// 2. Find the Player entity
Entity playerEntity = scene->FindEntityByTag("Player");
if (!playerEntity)
{
    auto playerView = scene->GetRegistry().view<PlayerComponent>();
    if (playerView.begin() != playerView.end())
    {
        playerEntity = Entity(*playerView.begin(), &scene->GetRegistry());
    }
}

if (!playerEntity || !playerEntity.HasComponent<TransformComponent>())
{
    return;
}

auto& playerTransform = playerEntity.GetComponent<TransformComponent>();

// 3. Handle Input (Orbit & Zoom)
bool isStandalone = !Application::Get().GetLayerStack().HasLayer("EditorLayer");
if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || isStandalone)
{
    Vector2 mouseDelta = Input::GetMouseDelta();
    camera.OrbitYaw -= mouseDelta.x * camera.LookSensitivity;
    camera.OrbitPitch -= mouseDelta.y * camera.LookSensitivity;

    // Clamp pitch
    camera.OrbitPitch = Clamp(camera.OrbitPitch, -10.0f, 85.0f);
}

float wheelMovement = Input::GetMouseWheelMove();
camera.OrbitDistance -= wheelMovement * 2.0f;
camera.OrbitDistance = Clamp(camera.OrbitDistance, 0.0f, 40.0f);

// 4. Calculate Position (Spherical Coordinates)
float yawRad = camera.OrbitYaw * DEG2RAD;
float pitchRad = camera.OrbitPitch * DEG2RAD;

float x = camera.OrbitDistance * cosf(pitchRad) * sinf(yawRad);
float y = camera.OrbitDistance * sinf(pitchRad);
float z = camera.OrbitDistance * cosf(pitchRad) * cosf(yawRad);

Vector3 targetPos = playerTransform.Translation;
targetPos.y += 1.5f; // Look at head level

auto& cameraTransform = GetComponent<TransformComponent>();
cameraTransform.Translation = Vector3Add(targetPos, {x, y, z});

// 5. Update Rotation
cameraTransform.Rotation.x = -camera.OrbitPitch;
cameraTransform.Rotation.y = camera.OrbitYaw;
cameraTransform.Rotation.z = 0.0f;

cameraTransform.RotationQuat = QuaternionFromEuler(
    cameraTransform.Rotation.x * DEG2RAD, cameraTransform.Rotation.y* DEG2RAD, cameraTransform.Rotation.z* DEG2RAD);
} // namespace CHEngine
}
;
} // namespace CHEngine

#endif // CH_CAMERA_CONTROLLER_H
