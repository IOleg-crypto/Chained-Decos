#ifndef IPLAYER_MEDIATOR_H
#define IPLAYER_MEDIATOR_H

#include <raylib.h>
#include <raymath.h>
#include <memory>
#include <Collision/System/CollisionSystem.h>
#include <Physics/Components/PhysicsComponent.h>
#include <CameraController/Core/CameraController.h>

class IPlayerMediator
{
public:
    virtual ~IPlayerMediator() = default;

    // Position & Size
    virtual Vector3 GetPlayerPosition() const = 0;
    virtual Vector3 GetPlayerSize() const = 0;
    virtual void SetPlayerPosition(const Vector3& pos) const = 0;

    // Physics
    virtual PhysicsComponent& GetPhysics() = 0;
    virtual const PhysicsComponent& GetPhysics() const = 0;

    // Movement & Speed
    virtual float GetSpeed() const = 0;
    virtual void SetSpeed(float speed) const = 0;
    virtual float GetRotationY() const = 0;
    virtual void SetRotationY(float rotationY) const = 0;
    virtual void ApplyJumpImpulse(float impulse) = 0;

    // Collision
    virtual const Collision& GetCollision() const = 0;
    virtual void SyncCollision() const = 0;

    // Camera
    virtual std::shared_ptr<CameraController> GetCameraController() const = 0;
};

#endif // IPLAYER_MEDIATOR_H

