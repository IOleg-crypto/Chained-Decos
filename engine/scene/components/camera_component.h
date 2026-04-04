#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include "engine/scene/scene_camera.h"
#include "engine/core/reflection.h"
#include <string>

namespace CHEngine
{
struct CameraComponent
{
    CHEngine::SceneCamera Camera;
    bool Primary = true;
    bool FixedAspectRatio = false;

    // Orbit camera settings
    bool IsOrbitCamera = false;
    std::string TargetEntityTag = "Player";
    float OrbitDistance = 10.0f;
    float OrbitYaw = 0.0f;
    float OrbitPitch = 20.0f;
    float LookSensitivity = 0.9f;

    CameraComponent() = default;
    CameraComponent(const CameraComponent&) = default;

    CH_REFLECT_BEGIN(CameraComponent)
        props.Header("General");
        props.Property("Primary", Primary);
        props.Property("Fixed Aspect Ratio", FixedAspectRatio);

        props.Nested("Projection", Camera);

        if (props.BeginGroup("Orbit Controller", IsOrbitCamera))
        {
            props.Property("Enabled", IsOrbitCamera);
            props.Property("Target Tag", TargetEntityTag);
            props.Property("Distance", OrbitDistance);
            props.Property("Yaw", OrbitYaw);
            props.Property("Pitch", OrbitPitch);
            props.Property("Sensitivity", LookSensitivity);
            props.EndGroup();
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
