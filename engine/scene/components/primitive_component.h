#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/core/reflection_rfl.h"

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
    std::shared_ptr<ModelAsset> Asset;

    static const char* GetStaticName() { return "PrimitiveComponent"; }
};

CH_MARK_RFL(PrimitiveComponent);

} // namespace CHEngine

#endif // CH_PRIMITIVE_COMPONENT_H
