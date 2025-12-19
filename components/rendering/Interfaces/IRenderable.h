#ifndef IRENDERABLE_H
#define IRENDERABLE_H

#include <raylib.h>
#include <string>

// Include necessary headers
#include "scene/resources/model/Core/Model.h"
#include "components/physics/collision/Core/CollisionManager.h"
#include "components/physics/collision/System/CollisionSystem.h"

class IRenderable
{
public:
    virtual ~IRenderable() = default;

    // Core rendering methods
    virtual void Update(CollisionManager& collisionManager) = 0;
    virtual void Render() = 0;

    // Methods for providing data needed by RenderManager
    virtual Vector3 GetPosition() const = 0;
    virtual BoundingBox GetBoundingBox() const = 0;
    virtual float GetRotationY() const = 0;
    virtual void UpdateCollision() = 0;
    virtual const Collision& GetCollision() const = 0;
    virtual Camera GetCamera() const = 0;
    virtual bool IsGrounded() const = 0;

    // For Menu, might need different methods, but since it's abstract, we can have defaults or separate interfaces
    // For now, assuming Player and Menu can implement these
};

#endif // IRENDERABLE_H



