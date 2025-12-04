#ifndef PHYSICS_H
#define PHYSICS_H

#include <raylib.h>
#include <raymath.h>

namespace Servers
{

struct Physics
{
    Vector3 Velocity = {0, 0, 0};
    float Gravity = 30.0f;
    float JumpStrength = 12.0f;
    bool IsGrounded = false;

    void ApplyGravity(float dt);
    void Jump();
    Vector3 GetMovement(float dt) const;
};

struct Bounds
{
    Vector3 Center = {0, 0, 0};
    Vector3 Size = {1, 2, 1};

    BoundingBox GetBoundingBox() const;
    void SetPosition(const Vector3 &pos);
};

} // namespace Servers

#endif // PHYSICS_H
