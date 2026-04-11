#ifndef CH_SCENE_CAMERA_H
#define CH_SCENE_CAMERA_H

#include <glm/glm.hpp>
#include <cstdint>
#include "engine/core/reflection.h"

namespace CHEngine
{

enum class ProjectionType
{
    Perspective = 0,
    Orthographic = 1
};

class SceneCamera
{
public:
    SceneCamera();
    virtual ~SceneCamera() = default;

    /** Sets a perspective projection using vertical field of view and clip planes. */
    void SetPerspective(float verticalFov, float nearClip, float farClip);
    /** Sets an orthographic projection using size and clip planes. */
    void SetOrthographic(float size, float nearClip, float farClip);

    /** Updates the cached projection when the viewport size changes. */
    void SetViewportSize(uint32_t width, uint32_t height);

    /** Returns the current perspective vertical field of view in radians. */
    float GetPerspectiveVerticalFOV() const
    {
        return m_PerspectiveFOV;
    }
    /** Updates the perspective vertical field of view in radians. */
    void SetPerspectiveVerticalFOV(float fov)
    {
        m_PerspectiveFOV = fov;
        RecalculateProjection();
    }
    /** Returns the near clip plane used for perspective projection. */
    float GetPerspectiveNearClip() const
    {
        return m_PerspectiveNear;
    }
    /** Updates the near clip plane used for perspective projection. */
    void SetPerspectiveNearClip(float nearClip)
    {
        m_PerspectiveNear = nearClip;
        RecalculateProjection();
    }
    /** Returns the far clip plane used for perspective projection. */
    float GetPerspectiveFarClip() const
    {
        return m_PerspectiveFar;
    }
    /** Updates the far clip plane used for perspective projection. */
    void SetPerspectiveFarClip(float farClip)
    {
        m_PerspectiveFar = farClip;
        RecalculateProjection();
    }

    /** Returns the orthographic projection size. */
    float GetOrthographicSize() const
    {
        return m_OrthographicSize;
    }
    /** Updates the orthographic projection size. */
    void SetOrthographicSize(float size)
    {
        m_OrthographicSize = size;
        RecalculateProjection();
    }
    /** Returns the near clip plane used for orthographic projection. */
    float GetOrthographicNearClip() const
    {
        return m_OrthographicNear;
    }
    /** Updates the near clip plane used for orthographic projection. */
    void SetOrthographicNearClip(float nearClip)
    {
        m_OrthographicNear = nearClip;
        RecalculateProjection();
    }
    /** Returns the far clip plane used for orthographic projection. */
    float GetOrthographicFarClip() const
    {
        return m_OrthographicFar;
    }
    /** Updates the far clip plane used for orthographic projection. */
    void SetOrthographicFarClip(float farClip)
    {
        m_OrthographicFar = farClip;
        RecalculateProjection();
    }

    /** Returns the active projection type. */
    ProjectionType GetProjectionType() const
    {
        return m_ProjectionType;
    }
    /** Switches between perspective and orthographic projection modes. */
    void SetProjectionType(ProjectionType type)
    {
        m_ProjectionType = type;
        RecalculateProjection();
    }

    /** Returns the cached projection matrix. */
    const glm::mat4& GetProjection() const
    {
        return m_Projection;
    }

    CH_REFLECT_BEGIN(SceneCamera)
        ProjectionType type = GetProjectionType();
        static const char* projTypes[] = { "Perspective", "Orthographic" };
        if (props.Enum("Projection", type, projTypes, 2) || props.GetMode() == ReflectionMode::Deserialize)
            SetProjectionType(type);

        if (type == ProjectionType::Perspective)
        {
            float fov = glm::degrees(m_PerspectiveFOV);
            if (props.Property("VerticalFOV", fov) || props.GetMode() == ReflectionMode::Deserialize)
                SetPerspectiveVerticalFOV(glm::radians(fov));
            
            float nearClipValue = m_PerspectiveNear;
            if (props.Property("Near", nearClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetPerspectiveNearClip(nearClipValue);
            
            float farClipValue = m_PerspectiveFar;
            if (props.Property("Far", farClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetPerspectiveFarClip(farClipValue);
        }
        else
        {
            float size = m_OrthographicSize;
            if (props.Property("Size", size) || props.GetMode() == ReflectionMode::Deserialize)
                SetOrthographicSize(size);
            
            float nearClipValue = m_OrthographicNear;
            if (props.Property("Near", nearClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetOrthographicNearClip(nearClipValue);
            
            float farClipValue = m_OrthographicFar;
            if (props.Property("Far", farClipValue) || props.GetMode() == ReflectionMode::Deserialize)
                SetOrthographicFarClip(farClipValue);
        }
    CH_REFLECT_END()

private:
    void RecalculateProjection();

private:
    ProjectionType m_ProjectionType = ProjectionType::Perspective;

    float m_PerspectiveFOV = glm::radians(60.0f);
    float m_PerspectiveNear = 0.01f;
    float m_PerspectiveFar = 100000.0f;

    float m_OrthographicSize = 10.0f;
    float m_OrthographicNear = -1.0f;
    float m_OrthographicFar = 1.0f;

    float m_AspectRatio = 0.0f;
    glm::mat4 m_Projection = glm::mat4(1.0f);
};

} // namespace CHEngine

#endif // CH_SCENE_CAMERA_H
