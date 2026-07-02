#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"

namespace Chained
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

    
    struct UI
    {
        UIMeta Type = {.Hint = PropertyMeta::WidgetHint::Enum};
        UIMeta Radius = {.Min = 0.01f, .Max = 1000.0f, .Speed = 0.05f};
        UIMeta InnerRadius = {.Min = 0.01f, .Max = 1000.0f, .Speed = 0.05f};
        UIMeta Height = {.Min = 0.01f, .Max = 1000.0f, .Speed = 0.05f};
        UIMeta Slices = {.Min = 3.0f, .Max = 256.0f, .Speed = 1.0f};
        UIMeta Stacks = {.Min = 2.0f, .Max = 256.0f, .Speed = 1.0f};
        UIMeta Dimensions = {.Speed = 0.1f};
    };
};

CH_MARK_RFL(PrimitiveComponent);

} // namespace Chained

#endif // CH_PRIMITIVE_COMPONENT_H
