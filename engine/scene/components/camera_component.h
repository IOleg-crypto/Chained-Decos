#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include "engine/scene/camera.h"
#include "engine/core/reflection_rfl.h"
#include <string>

namespace CHEngine
{
struct CameraComponent
{
    CHEngine::Camera Camera;
    bool Primary = true;
    bool FixedAspectRatio = false;

    // Orbit camera settings
    bool IsOrbitCamera = false;
    std::string TargetEntityTag = "Player";
    float OrbitDistance = 10.0f;
    float OrbitYaw = 0.0f;
    float OrbitPitch = 20.0f;
    float LookSensitivity = 0.9f;



    static const char* GetStaticName() { return "CameraComponent"; }
};

CH_MARK_RFL(CameraComponent);

} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
