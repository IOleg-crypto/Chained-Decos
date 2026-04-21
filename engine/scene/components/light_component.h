#ifndef CH_LIGHT_COMPONENT_H
#define CH_LIGHT_COMPONENT_H


#include "engine/core/reflection.h"

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


    CH_REFLECT_BEGIN(LightComponent)
        props.Header("General");
        static const char* lightTypeStrings[] = { "Point", "Spot", "Directional" };
        props.Enum("Type", Type, lightTypeStrings, 3);
        props.Property("Color", LightColor);
        props.Property("Intensity", Intensity, PropertyMeta(0.0f, 1000.0f, 1.0f));
        
        if (props.BeginGroup("Parameters"))
        {
            props.Property("Radius", Radius, PropertyMeta(1.0f, 1000.0f, 1.0f));
            
            // Always show Spot fields if the light is a Spot light.
            if (Type == LightType::Spot)
            {
                props.Property("InnerCutoff", InnerCutoff, PropertyMeta(0.0f, 90.0f, 0.5f));
                props.Property("OuterCutoff", OuterCutoff, PropertyMeta(0.0f, 90.0f, 0.5f));
            }
            else if (props.GetMode() != CHEngine::ReflectionMode::UI)
            {
                // Still serialize them for other types to avoid data loss.
                props.Property("InnerCutoff", InnerCutoff);
                props.Property("OuterCutoff", OuterCutoff);
            }
            props.EndGroup();
        }

        props.Separator();
        props.Property("Shadows", Shadows);
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_LIGHT_COMPONENT_H
