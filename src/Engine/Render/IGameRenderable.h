#ifndef IGAMERENDERABLE_H
#define IGAMERENDERABLE_H

#include <raylib.h>
#include "Model/Model.h"
#include "Collision/CollisionManager.h"
#include "Collision/CollisionSystem.h"

// Interface for objects that require full game rendering (Player, NPC, etc.)
// Follows Interface Segregation Principle - only methods needed for game objects
struct IGameRenderable
{
    virtual ~IGameRenderable() = default;

    // Update object state
    virtual void Update(CollisionManager& collisionManager) = 0;
    
    // Rendering methods
    virtual Vector3 GetPosition() const = 0;
    virtual BoundingBox GetBoundingBox() const = 0;
    virtual float GetRotationY() const = 0;
    virtual void UpdateCollision() = 0;
    virtual const Collision& GetCollision() const = 0;
    virtual Camera GetCamera() const = 0;
    virtual bool IsGrounded() const = 0;
};

#endif // IGAMERENDERABLE_H
