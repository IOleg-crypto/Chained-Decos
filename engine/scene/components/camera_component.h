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
            props.Property("Distance", OrbitDistance, PropertyMeta(1.0f, 100.0f, 0.5f));
            props.Property("Yaw", OrbitYaw, PropertyMeta(-360.0f, 360.0f, 1.0f));
            props.Property("Pitch", OrbitPitch, PropertyMeta(-90.0f, 90.0f, 1.0f));
            props.Property("Sensitivity", LookSensitivity, PropertyMeta(0.1f, 5.0f, 0.1f));
            props.EndGroup();
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
