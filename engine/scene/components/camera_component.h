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
        CH_HEADER(props, "General");
        CH_PROP(props, Primary);
        CH_PROP(props, FixedAspectRatio);

        CH_NESTED_NAMED(props, "Projection", Camera);

        if (CH_BEGIN_GROUP(props, "OrbitController", IsOrbitCamera))
        {
            CH_PROP(props, IsOrbitCamera);
            CH_PROP(props, TargetEntityTag);
            CH_PROP_META(props, OrbitDistance, PropertyMeta(1.0f, 100.0f, 0.5f));
            CH_PROP_META(props, OrbitYaw, PropertyMeta(-360.0f, 360.0f, 1.0f));
            CH_PROP_META(props, OrbitPitch, PropertyMeta(-90.0f, 90.0f, 1.0f));
            CH_PROP_META(props, LookSensitivity, PropertyMeta(0.1f, 5.0f, 0.1f));
            CH_END_GROUP(props);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
