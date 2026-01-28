#ifndef CH_LIGHTING_COMPONENTS_H
#define CH_LIGHTING_COMPONENTS_H

#include "raylib.h"
#include "string"

namespace CHEngine
{
struct PointLightComponent
{
    Color LightColor = WHITE;
    float Radiance = 1.0f;
    float Radius = 10.0f;
    float Falloff = 1.0f;

    PointLightComponent() = default;
    PointLightComponent(const PointLightComponent &) = default;
};

struct SkyboxComponent
{
    std::string TexturePath;
    float Exposure = 1.0f;
    float Brightness = 0.0f;
    float Contrast = 1.0f;

    SkyboxComponent() = default;
    SkyboxComponent(const SkyboxComponent &) = default;
};
} // namespace CHEngine

#endif // CH_LIGHTING_COMPONENTS_H
