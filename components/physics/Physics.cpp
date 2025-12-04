#include "Physics.h"

namespace Servers
{

BoundingBox Bounds::ToBoundingBox() const
{
    Vector3 half = {size.x / 2.0f, size.y / 2.0f, size.z / 2.0f};
    return {{center.x - half.x, center.y - half.y, center.z - half.z},
            {center.x + half.x, center.y + half.y, center.z + half.z}};
}

void Physics::ApplyGravity(float delta_time)
{
    if (!is_grounded)
    {
        velocity.y -= gravity * delta_time;
    }
}

void Physics::Jump()
{
    if (is_grounded)
    {
        velocity.y = jump_force;
        is_grounded = false;
    }
}

Vector3 Physics::GetMovement(float delta_time) const
{
    return {velocity.x * delta_time, velocity.y * delta_time, velocity.z * delta_time};
}

void Physics::Reset()
{
    velocity = {0, 0, 0};
    is_grounded = false;
}

} // namespace Servers
