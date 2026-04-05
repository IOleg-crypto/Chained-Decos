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

    void SetPerspective(float verticalFov, float nearClip, float farClip);
    void SetOrthographic(float size, float nearClip, float farClip);

    void SetViewportSize(uint32_t width, uint32_t height);

    float GetPerspectiveVerticalFOV() const
    {
        return m_PerspectiveFOV;
    }
    void SetPerspectiveVerticalFOV(float fov)
    {
        m_PerspectiveFOV = fov;
        RecalculateProjection();
    }
    float GetPerspectiveNearClip() const
    {
        return m_PerspectiveNear;
    }
    void SetPerspectiveNearClip(float nearClip)
    {
        m_PerspectiveNear = nearClip;
        RecalculateProjection();
    }
    float GetPerspectiveFarClip() const
    {
        return m_PerspectiveFar;
    }
    void SetPerspectiveFarClip(float farClip)
    {
        m_PerspectiveFar = farClip;
        RecalculateProjection();
    }

    float GetOrthographicSize() const
    {
        return m_OrthographicSize;
    }
    void SetOrthographicSize(float size)
    {
        m_OrthographicSize = size;
        RecalculateProjection();
    }
    float GetOrthographicNearClip() const
    {
        return m_OrthographicNear;
    }
    void SetOrthographicNearClip(float nearClip)
    {
        m_OrthographicNear = nearClip;
        RecalculateProjection();
    }
    float GetOrthographicFarClip() const
    {
        return m_OrthographicFar;
    }
    void SetOrthographicFarClip(float farClip)
    {
        m_OrthographicFar = farClip;
        RecalculateProjection();
    }

    ProjectionType GetProjectionType() const
    {
        return m_ProjectionType;
    }
    void SetProjectionType(ProjectionType type)
    {
        m_ProjectionType = type;
        RecalculateProjection();
    }

    const glm::mat4& GetProjection() const
    {
        return m_Projection;
    }

    CH_REFLECT_BEGIN(SceneCamera)
        ProjectionType type = GetProjectionType();
        static const char* projTypes[] = { "Perspective", "Orthographic" };
        if (props.Enum("Projection", type, projTypes, 2))
            SetProjectionType(type);

        if (type == ProjectionType::Perspective)
        {
            float fov = glm::degrees(m_PerspectiveFOV);
            if (props.Property("VerticalFOV", fov))
                SetPerspectiveVerticalFOV(glm::radians(fov));
            
            float n = m_PerspectiveNear;
            if (props.Property("Near", n))
                SetPerspectiveNearClip(n);
            
            float f = m_PerspectiveFar;
            if (props.Property("Far", f))
                SetPerspectiveFarClip(f);
        }
        else
        {
            float size = m_OrthographicSize;
            if (props.Property("Size", size))
                SetOrthographicSize(size);
            
            float n = m_OrthographicNear;
            if (props.Property("Near", n))
                SetOrthographicNearClip(n);
            
            float f = m_OrthographicFar;
            if (props.Property("Far", f))
                SetOrthographicFarClip(f);
        }
    CH_REFLECT_END()

private:
    void RecalculateProjection();

private:
    ProjectionType m_ProjectionType = ProjectionType::Perspective;

    float m_PerspectiveFOV = glm::radians(60.0f);
    float m_PerspectiveNear = 0.01f;
    float m_PerspectiveFar = 1000.0f;

    float m_OrthographicSize = 10.0f;
    float m_OrthographicNear = -1.0f;
    float m_OrthographicFar = 1.0f;

    float m_AspectRatio = 0.0f;
    glm::mat4 m_Projection = glm::mat4(1.0f);
};

} // namespace CHEngine

#endif // CH_SCENE_CAMERA_H
