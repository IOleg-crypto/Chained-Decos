#ifndef CH_LIGHT_COMPONENT_H
#define CH_LIGHT_COMPONENT_H

#include "raylib.h"

namespace CHEngine
{
enum class LightType
{
    Point = 0,
    Spot = 1
};

struct LightComponent
{
    LightType Type = LightType::Point;
    Color LightColor = WHITE;
    float Intensity = 100.0f;
    float Radius = 100.0f;     // Also used as Range for Spot lights
    float InnerCutoff = 15.0f; // Spot light only (degrees)
    float OuterCutoff = 20.0f; // Spot light only (degrees)
    bool Shadows = false;      // Future proofing

    LightComponent() = default;
    LightComponent(const LightComponent&) = default;
};
} // namespace CHEngine

#endif // CH_LIGHT_COMPONENT_H
