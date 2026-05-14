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
        CH_HEADER(props, "General");
        static const char* lightTypeStrings[] = { "Point", "Spot", "Directional" };
        CH_ENUM(props, Type, lightTypeStrings);
        CH_PROP(props, LightColor);
        CH_PROP_META(props, Intensity, PropertyMeta(0.0f, 1000.0f, 1.0f));
        
        if (CH_BEGIN_GROUP(props, "Parameters", true))
        {
            CH_PROP_META(props, Radius, PropertyMeta(1.0f, 1000.0f, 1.0f));
            
            // Always show Spot fields if the light is a Spot light.
            if (Type == LightType::Spot)
            {
                CH_PROP_META(props, InnerCutoff, PropertyMeta(0.0f, 90.0f, 0.5f));
                CH_PROP_META(props, OuterCutoff, PropertyMeta(0.0f, 90.0f, 0.5f));
            }
            else if (props.GetMode() != CHEngine::ReflectionMode::UI)
            {
                // Still serialize them for other types to avoid data loss.
                CH_PROP(props, InnerCutoff);
                CH_PROP(props, OuterCutoff);
            }
            CH_END_GROUP(props);
        }

        CH_SEPARATOR(props);
        CH_PROP(props, Shadows);
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_LIGHT_COMPONENT_H
