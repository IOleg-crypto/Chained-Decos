#ifndef IPLAYER_H
#define IPLAYER_H

#include <components/physics/collision/system/collisionSystem.h>
#include <components/physics/dynamics/components/PhysicsComponent.h>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <scene/camera/CameraController.h>

// Minimal Player interface
// Essential API only - 8 methods (down from 30+)
// Engine depends on this interface, not concrete Player class.
class IPlayer
{
public:
    virtual ~IPlayer() = default;

    // Position
    virtual Vector3 GetPosition() const = 0;
    virtual void SetPosition(const Vector3 &pos) = 0;

    // Properties
    virtual float GetSpeed() const = 0;
    virtual void SetSpeed(float speed) const = 0;
    virtual float GetRotationY() const = 0;
    virtual void SetRotationY(float rotation) const = 0;

    // Core update
    virtual void Update(float deltaTime) = 0;

    // Camera
    virtual Camera3D &GetCamera() = 0;

    // debug/Cheat
    virtual void SetNoclip(bool enabled) = 0;
    virtual bool IsNoclip() const = 0;

    // Position & Size (from IPlayerMediator)
    virtual Vector3 GetPlayerPosition() const = 0;
    virtual Vector3 GetPlayerSize() const = 0;
    virtual void SetPlayerPosition(const Vector3 &pos) const = 0;

    // Physics
    virtual PhysicsComponent &GetPhysics() = 0;
    virtual const PhysicsComponent &GetPhysics() const = 0;

    // Movement
    virtual void ApplyJumpImpulse(float impulse) = 0;

    // Collision
    virtual const Collision &GetCollision() const = 0;
    virtual void SyncCollision() const = 0;
    virtual void InitializeCollision() = 0;

    // Camera (detailed)
    virtual std::shared_ptr<CameraController> GetCameraController() const = 0;

    // Bounding box
    virtual BoundingBox GetPlayerBoundingBox() const = 0;
};

#endif // IPLAYER_H
