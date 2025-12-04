#ifndef SERVERS_PHYSICS_H
#define SERVERS_PHYSICS_H

#include <raylib.h>

namespace Servers
{

struct Bounds
{
    Vector3 center = {0, 0, 0};
    Vector3 size = {1, 1, 1};

    BoundingBox ToBoundingBox() const;
};

class Physics
{
public:
    Physics() = default;
    ~Physics() = default;

    // State
    Vector3 velocity = {0, 0, 0};
    bool is_grounded = false;

    // Constants
    float gravity = 20.0f;
    float jump_force = 10.0f;

    // Methods
    void ApplyGravity(float delta_time);
    void Jump();
    Vector3 GetMovement(float delta_time) const;
    void Reset();
};

} // namespace Servers

#endif // SERVERS_PHYSICS_H
