#ifndef CH_LIGHTING_COMPONENTS_H
#define CH_LIGHTING_COMPONENTS_H

#include "raylib.h"
#include <string>

namespace CHEngine
{
struct PointLightComponent
{
    Color LightColor = WHITE;
    float Intensity = 1.0f;
    float Radius = 10.0f;
    float Falloff = 1.0f;

    PointLightComponent() = default;
    PointLightComponent(const PointLightComponent &) = default;
};

struct SpotLightComponent
{
    Color LightColor = WHITE;
    float Intensity = 1.0f;
    float Range = 20.0f;
    float InnerCutoff = 15.0f; // Angles in degrees
    float OuterCutoff = 20.0f;

    SpotLightComponent() = default;
    SpotLightComponent(const SpotLightComponent &) = default;
};

} // namespace CHEngine

#endif // CH_LIGHTING_COMPONENTS_H
