#ifndef IPLAYER_H
#define IPLAYER_H

#include <raylib.h>

/**
 * @brief Minimal Player interface
 *
 * Essential API only - 8 methods (down from 30+)
 * Engine depends on this interface, not concrete Player class.
 */
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
};

#endif // IPLAYER_H
