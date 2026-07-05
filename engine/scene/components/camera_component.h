#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include "engine/scene/camera.h"
#include "engine/reflection/reflection_rfl.h"
#include <string>

namespace Chained
{
struct CameraComponent
{
    Camera Camera;
    bool Primary = true;
    bool FixedAspectRatio = false;

    // Orbit camera settings
    bool IsOrbitCamera = false;
    std::string TargetEntityTag = "Player";
    float OrbitDistance = 10.0f;
    float OrbitYaw = 0.0f;
    float OrbitPitch = 20.0f;
    float LookSensitivity = 0.9f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    float verticalFov = 60.0f;
    float orthographicSize = 10.0f;

    static const char* GetStaticName() { return "CameraComponent"; }

    
    struct UI
    {
        UIMeta TargetEntityTag = {.Tooltip = "Tag of the entity to follow (Orbit Camera)"};
        UIMeta OrbitDistance = {.Min = 1.0f, .Max = 500.0f, .Speed = 0.5f};
        UIMeta OrbitYaw = {.Speed = 1.0f};
        UIMeta OrbitPitch = {.Min = -89.0f, .Max = 89.0f, .Speed = 1.0f};
        UIMeta LookSensitivity = {.Min = 0.1f, .Max = 10.0f, .Speed = 0.05f};
        UIMeta nearClip = {.Min = 0.01f, .Max = 100.0f, .Speed = 0.01f};
        UIMeta farClip = {.Min = 1.0f, .Max = 1000.0f, .Speed = 1.0f};
        UIMeta verticalFov = {.Min = 1.0f, .Max = 180.0f, .Speed = 0.5f};
        UIMeta orthographicSize = {.Min = 1.0f, .Max = 100.0f, .Speed = 0.5f};
    };
};

CH_MARK_RFL(CameraComponent);

} // namespace Chained

#endif // CH_CAMERA_COMPONENT_H
