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
        props.Header("Shape Selection");
        const char* primitiveTypes[] = {"None", "Cube",  "Sphere", "Plane",     "Cylinder",
                                        "Cone", "Torus", "Knot",   "Hemisphere"};
        
        if (props.Enum("Primitive Type", Type, primitiveTypes, (int)CH_ARRAY_SIZE(primitiveTypes)))
        {
            Dirty = true;
            Asset = nullptr;
        }

        if (Type == PrimitiveType::None) return;

        if (props.BeginGroup("Parameters"))
        {
            if (Type == PrimitiveType::Cube || Type == PrimitiveType::Plane)
            {
                props.Property("Dimensions", Dimensions);
            }
            else
            {
                props.Property("Radius", Radius);
                if (Type == PrimitiveType::Torus)
                    props.Property("Inner Radius", InnerRadius);
                
                if (Type == PrimitiveType::Cylinder || Type == PrimitiveType::Cone)
                    props.Property("Height", Height);

                props.Property("Slices", Slices);
                props.Property("Stacks", Stacks);
            }
            props.EndGroup();
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
