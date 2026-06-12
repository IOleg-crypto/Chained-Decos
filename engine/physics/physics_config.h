#pragma once
#include <glm/glm.hpp>

namespace Chained
{
    /**
     * @brief Physics simulation settings.
     * Allows decoupling Physics from Project settings.
     */
    struct PhysicsConfig
    {
        float Gravity = -9.81f;
        float FixedTimestep = 1.0f / 60.0f;
        uint32_t PositionIterations = 2;
        uint32_t VelocityIterations = 6;
    };
}
