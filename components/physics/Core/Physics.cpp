#include "Physics.h"

namespace Servers
{

void Physics::ApplyGravity(float dt)
{
    if (!IsGrounded)
    {
        Velocity.y -= Gravity * dt;
        if (Velocity.y < -60.0f)
        {
            Velocity.y = -60.0f;
        }
    }
}

void Physics::Jump()
{
    if (IsGrounded)
    {
        Velocity.y = JumpStrength;
        IsGrounded = false;
    }
}

Vector3 Physics::GetMovement(float dt) const
{
    return Vector3Scale(Velocity, dt);
}

BoundingBox Bounds::GetBoundingBox() const
{
    Vector3 half = Vector3Scale(Size, 0.5f);
    return {Vector3Subtract(Center, half), Vector3Add(Center, half)};
}

void Bounds::SetPosition(const Vector3 &pos)
{
    Center = pos;
}

} // namespace Servers
