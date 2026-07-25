#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"
#include "engine/graphics/api/renderer_types.h"

namespace Chained
{

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

    // Material Properties (Serialized)
    std::string AlbedoPath;
    glm::vec4 AlbedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
    std::string NormalPath;
    std::string MetallicRoughnessPath;
    std::string EmissivePath;
    glm::vec4 EmissiveColor = {0.0f, 0.0f, 0.0f, 1.0f};
    float EmissiveIntensity = 0.0f;
    float Metalness = 0.0f;
    float Roughness = 0.5f;
    bool Transparent = false;
    float Alpha = 1.0f;

    // NOTE: Runtime-only state (Dirty flag, Asset ptr) lives in PrimitiveRuntimeState
    //       to keep this struct aggregate-reflectable by reflect-cpp.

    PrimitiveComponent() = default;

    static const char* GetStaticName() { return "PrimitiveComponent"; }

    Material GetMaterial() const
    {
        Material mat;
        mat.AlbedoPath = AlbedoPath;
        mat.AlbedoColor = AlbedoColor;
        mat.NormalPath = NormalPath;
        mat.MetallicRoughnessPath = MetallicRoughnessPath;
        mat.EmissivePath = EmissivePath;
        mat.EmissiveColor = EmissiveColor;
        mat.EmissiveIntensity = EmissiveIntensity;
        mat.Metalness = Metalness;
        mat.Roughness = Roughness;
        mat.Transparent = Transparent;
        mat.Alpha = Alpha;
        return mat;
    }

    void SetMaterial(const Material& mat)
    {
        AlbedoPath = mat.AlbedoPath;
        AlbedoColor = mat.AlbedoColor;
        NormalPath = mat.NormalPath;
        MetallicRoughnessPath = mat.MetallicRoughnessPath;
        EmissivePath = mat.EmissivePath;
        EmissiveColor = mat.EmissiveColor;
        EmissiveIntensity = mat.EmissiveIntensity;
        Metalness = mat.Metalness;
        Roughness = mat.Roughness;
        Transparent = mat.Transparent;
        Alpha = mat.Alpha;
    }

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
