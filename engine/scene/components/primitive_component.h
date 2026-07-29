#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/reflection/reflection.h"
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

    // Geometry Parameters
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

    PrimitiveComponent() = default;

    static const char* GetStaticName()
    {
        return "PrimitiveComponent";
    }

    template <typename T_Archive> void Reflect(Properties<T_Archive>& props)
    {
        static const char* primitiveTypeNames[] = {"None", "Cube",  "Sphere", "Plane",     "Cylinder",
                                                   "Cone", "Torus", "Knot",   "Hemisphere"};

        props.Enum("Type", Type, primitiveTypeNames, 9);
        props.Property("Radius", Radius);
        props.Property("InnerRadius", InnerRadius);
        props.Property("Height", Height);
        props.Property("Slices", Slices);
        props.Property("Stacks", Stacks);
        props.Property("Dimensions", Dimensions);

        // Material properties belong to Material Editor UI, but must be serialized during Save/Load
        if (props.GetMode() != ReflectionMode::UI)
        {
            props.Property("AlbedoPath", AlbedoPath);
            props.Property("AlbedoColor", AlbedoColor);
            props.Property("NormalPath", NormalPath);
            props.Property("MetallicRoughnessPath", MetallicRoughnessPath);
            props.Property("EmissivePath", EmissivePath);
            props.Property("EmissiveColor", EmissiveColor);
            props.Property("EmissiveIntensity", EmissiveIntensity);
            props.Property("Metalness", Metalness);
            props.Property("Roughness", Roughness);
            props.Property("Transparent", Transparent);
            props.Property("Alpha", Alpha);
        }
    }

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
};

} // namespace Chained

#endif // CH_PRIMITIVE_COMPONENT_H
