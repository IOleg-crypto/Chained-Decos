#ifndef CH_LIGHT_COMPONENT_H
#define CH_LIGHT_COMPONENT_H

#include "engine/core/base.h"

namespace CHEngine
{
enum class LightType
{
    Point = 0,
    Spot = 1,
    Directional = 2
};

struct LightComponent
{
    LightType Type = LightType::Point;
    Color LightColor = Color::White();
    float Intensity = 100.0f;
    float Radius = 100.0f;     // Also used as Range for Spot lights
    float InnerCutoff = 15.0f; // Spot light only (degrees)
    float OuterCutoff = 20.0f; // Spot light only (degrees)
    bool Shadows = false;      // Future proofing

    LightComponent() = default;
    LightComponent(const LightComponent&) = default;

    static const char* GetStaticName() { return "LightComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, LightComponent& component)
    {
        archive.Property("Type", (int&)component.Type)
            .Property("LightColor", component.LightColor)
            .Property("Intensity", component.Intensity)
            .Property("Radius", component.Radius)
            .Property("InnerCutoff", component.InnerCutoff)
            .Property("OuterCutoff", component.OuterCutoff);
    }
};
} // namespace CHEngine

#endif // CH_LIGHT_COMPONENT_H
