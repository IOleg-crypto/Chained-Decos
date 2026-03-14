#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/core/base.h"
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
    Vector3 Dimensions = {1.0f, 1.0f, 1.0f};

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
};

} // namespace CHEngine

#endif // CH_PRIMITIVE_COMPONENT_H
