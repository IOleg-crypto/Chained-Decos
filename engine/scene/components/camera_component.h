#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include "engine/scene/scene_camera.h"
#include <string>

namespace CHEngine
{
struct CameraComponent
{
    CHEngine::SceneCamera Camera;
    bool Primary = true; // If true, this is the main game camera
    bool FixedAspectRatio = false;

    // Orbit camera settings (maintained for the controller script)
    bool IsOrbitCamera = false;
    std::string TargetEntityTag = "Player";
    float OrbitDistance = 10.0f;
    float OrbitYaw = 0.0f;
    float OrbitPitch = 20.0f;
    float LookSensitivity = 0.9f;

    CameraComponent() = default;
    CameraComponent(const CameraComponent&) = default;
};

} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
