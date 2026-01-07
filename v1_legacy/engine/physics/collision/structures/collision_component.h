#ifndef CD_COMPONENTS_PHYSICS_COLLISION_STRUCTURES_COLLISION_COMPONENT_H
#define CD_COMPONENTS_PHYSICS_COLLISION_STRUCTURES_COLLISION_COMPONENT_H

#include <functional>
#include <memory>
#include <raylib.h>

namespace CHEngine
{
// Simple CollisionComponent class for testing
class CollisionComponent
{
public:
    CollisionComponent() = default;
    ~CollisionComponent() = default;

    void SetBoundingBox(const BoundingBox &box)
    {
        m_boundingBox = box;
    }
    const BoundingBox &GetBoundingBox() const
    {
        return m_boundingBox;
    }

    void SetSphereCollision(float radius)
    {
        m_radius = radius;
        m_isSphere = true;
    }
    float GetRadius() const
    {
        return m_radius;
    }
    bool IsSphere() const
    {
        return m_isSphere;
    }

    void SetPosition(const Vector3 &pos)
    {
        m_position = pos;
    }
    const Vector3 &GetPosition() const
    {
        return m_position;
    }

    void SetCollisionCallback(std::function<void()> callback)
    {
        m_callback = callback;
    }
    std::function<void()> &GetCollisionCallback()
    {
        return m_callback;
    }

private:
    BoundingBox m_boundingBox{0};
    Vector3 m_position{0};
    float m_radius = 0.0f;
    bool m_isSphere = false;
    std::function<void()> m_callback;
};

} // namespace CHEngine

#endif // CD_COMPONENTS_PHYSICS_COLLISION_STRUCTURES_COLLISION_COMPONENT_H
