#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/core/reflection.h"
#include <memory>

namespace CHEngine
{
class ModelAsset;

enum class PrimitiveType : uint8_t
{
    None = 0,
    Cube,
    Sphere,
    Plane,
    Cylinder,
    Cone,
    Torus,
    Knot,
    Hemisphere
};

struct PrimitiveComponent
{
    PrimitiveType Type = PrimitiveType::None;

    // Parameters
    float Radius = 0.5f;
    float InnerRadius = 0.2f;
    float Height = 1.0f;
    int Slices = 16;
    int Stacks = 16;
    glm::vec3 Dimensions = {1.0f, 1.0f, 1.0f};

    // Internal state
    bool Dirty = false;

    // Runtime-cached asset reference
    std::shared_ptr<ModelAsset> Asset;

    PrimitiveComponent() = default;
    PrimitiveComponent(const PrimitiveComponent&) = default;
    PrimitiveComponent(PrimitiveType type)
        : Type(type)
    {
    }

    CH_REFLECT_BEGIN(PrimitiveComponent)
        CH_HEADER(props, "Shape Selection");
        const char* primitiveTypes[] = {"None", "Cube",  "Sphere", "Plane",     "Cylinder",
                                        "Cone", "Torus", "Knot",   "Hemisphere"};
        
        if (CH_ENUM_NAMED(props, "Primitive Type", Type, primitiveTypes))
        {
            Dirty = true;
            Asset = nullptr;
        }

        if (Type == PrimitiveType::None) return;

        if (CH_BEGIN_GROUP(props, "Parameters", true))
        {
            if (Type == PrimitiveType::Cube || Type == PrimitiveType::Plane)
            {
                CH_PROP_META(props, Dimensions, PropertyMeta(0.01f, 100.0f, 0.1f));
            }
            else
            {
                CH_PROP_META(props, Radius, PropertyMeta(0.01f, 50.0f, 0.1f));
                if (Type == PrimitiveType::Torus)
                    CH_PROP_META_NAMED(props, "Inner Radius", InnerRadius, PropertyMeta(0.01f, 25.0f, 0.1f));
                
                if (Type == PrimitiveType::Cylinder || Type == PrimitiveType::Cone)
                    CH_PROP_META(props, Height, PropertyMeta(0.1f, 100.0f, 0.1f));

                CH_PROP_META(props, Slices, PropertyMeta(3.0f, 256.0f, 1.0f));
                CH_PROP_META(props, Stacks, PropertyMeta(3.0f, 256.0f, 1.0f));
            }
            CH_END_GROUP(props);
        }

        if (props.GetMode() == ReflectionMode::Deserialize && props.HasChanged())
        {
            Dirty = true;
            Asset = nullptr;
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_PRIMITIVE_COMPONENT_H
