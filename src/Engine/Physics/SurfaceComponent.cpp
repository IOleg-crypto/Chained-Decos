#include "SurfaceComponent.h" 

SurfaceComponent::SurfaceComponent(SurfaceType type): m_surfaceType(type) {}

SurfaceType SurfaceComponent::GetSurfaceType() const {return m_surfaceType; }
void SurfaceComponent::SetSurfaceType(SurfaceType type) { m_surfaceType = type; }
float SurfaceComponent::GetFriction() const { return m_friction; }

void SurfaceComponent::SetFriction(float friction) { m_friction = friction; }

float SurfaceComponent::GetBounciness() const { return m_bounciness; }
void SurfaceComponent::SetBounciness(float bounciness)
{
    m_bounciness = bounciness;
}
