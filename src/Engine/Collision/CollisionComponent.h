#pragma once

#include <vector>
#include "../Core/Component.h"
#include "CollisionSystem.h"
#include "../Physics/SurfaceComponent.h"

namespace Engine {
namespace Collision {

class CollisionComponent : public Core::Component {
public:
    explicit CollisionComponent() = default;

    void Start() override {
        UpdateCollisionFromEntity();
    }

    void Update(float deltaTime) override {
        if (m_autoUpdate) {
            UpdateCollisionFromEntity();
        }
    }

    // Collision type management
    void SetCollisionType(CollisionType type) {
        m_collisionData.SetCollisionType(type);
    }

    CollisionType GetCollisionType() const {
        return m_collisionData.GetCollisionType();
    }

    // Surface interaction
    Physics::SurfaceType GetSurfaceType() const {
        if (auto surface = GetOwner()->GetComponent<Physics::SurfaceComponent>()) {
            return surface->GetSurfaceType();
        }
        return Physics::SurfaceType::Default;
    }

    // Collision data access
    const Collision& GetCollisionData() const { return m_collisionData; }
    void SetAutoUpdate(bool autoUpdate) { m_autoUpdate = autoUpdate; }

    // Collision response
    virtual void OnCollisionEnter(const CollisionComponent* other) {}
    virtual void OnCollisionStay(const CollisionComponent* other) {}
    virtual void OnCollisionExit(const CollisionComponent* other) {}

protected:
    virtual void UpdateCollisionFromEntity() {}

private:
    Collision m_collisionData;
    bool m_autoUpdate = true;
    std::vector<const CollisionComponent*> m_currentCollisions;
};

}} // namespace Engine::Collision
