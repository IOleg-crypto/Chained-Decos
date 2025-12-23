#ifndef VELOCITY_COMPONENT_H
#define VELOCITY_COMPONENT_H

#include <raylib.h>

namespace CHEngine
{
struct VelocityComponent
{
    Vector3 velocity = {0, 0, 0};
    Vector3 acceleration = {0, 0, 0};
    float drag = 0.1f; // Air resistance
};
} // namespace CHEngine

#endif // VELOCITY_COMPONENT_H
