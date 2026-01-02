#ifndef CD_COMPONENTS_PHYSICS_DYNAMICS_COMPONENTS_SURFACE_COMPONENT_H
#define CD_COMPONENTS_PHYSICS_DYNAMICS_COMPONENTS_SURFACE_COMPONENT_H

#include <memory>
#include <vector>

enum class SurfaceType : uint8_t
{
    Default,
    Ground,
    Wall,
    Platform,
    Water,
    Ice,
    Mud
};

class SurfaceComponent
{
public:
    explicit SurfaceComponent(SurfaceType type = SurfaceType::Default);

    [[nodiscard]] SurfaceType GetSurfaceType() const;
    void SetSurfaceType(SurfaceType type);

    // Surface properties
    float GetFriction() const;
    void SetFriction(float friction);

    float GetBounciness() const;
    void SetBounciness(float bounciness);

private:
    SurfaceType m_surfaceType;
    float m_friction = 1.0f;
    float m_bounciness = 0.0f;
};

#endif




