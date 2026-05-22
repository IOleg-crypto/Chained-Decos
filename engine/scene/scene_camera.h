#ifndef CH_SCENE_CAMERA_H
#define CH_SCENE_CAMERA_H

#include "engine/scene/camera.h"
#include "engine/core/reflection.h"

namespace CHEngine
{

class SceneCamera : public Camera
{
public:
    SceneCamera();
    virtual ~SceneCamera() override = default;

    void SetPerspective(float verticalFov, float nearClip, float farClip);
    void SetOrthographic(float size, float nearClip, float farClip);

    CH_REFLECT_BEGIN(SceneCamera)
        ProjectionType type = GetProjectionType();
        static const char* projTypes[] = { "Perspective", "Orthographic" };
        if (CH_ENUM_NAMED(props, "Projection", type, projTypes) || props.GetMode() == ReflectionMode::Deserialize)
            SetProjectionType(type);

        if (type == ProjectionType::Perspective)
        {
            float fov = glm::degrees(GetPerspectiveVerticalFOV());
            if (CH_PROP_NAMED(props, "VerticalFOV", fov) || props.GetMode() == ReflectionMode::Deserialize)
                SetPerspectiveVerticalFOV(glm::radians(fov));
            
            float nearClipValue = GetPerspectiveNearClip();
            if (CH_PROP_NAMED(props, "Near", nearClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetPerspectiveNearClip(nearClipValue);
            
            float farClipValue = GetPerspectiveFarClip();
            if (CH_PROP_NAMED(props, "Far", farClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetPerspectiveFarClip(farClipValue);
        }
        else
        {
            float size = GetOrthographicSize();
            if (CH_PROP_NAMED(props, "Size", size) || props.GetMode() == ReflectionMode::Deserialize)
                SetOrthographicSize(size);
            
            float nearClipValue = GetOrthographicNearClip();
            if (CH_PROP_NAMED(props, "Near", nearClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetOrthographicNearClip(nearClipValue);
            
            float farClipValue = GetOrthographicFarClip();
            if (CH_PROP_NAMED(props, "Far", farClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetOrthographicFarClip(farClipValue);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_SCENE_CAMERA_H
