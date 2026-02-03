#include <glm/glm.hpp>
#include <raylib.h>
#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

// Removed redundant include: engine/core/math_types.h
#include <string>

namespace CHEngine
{
struct MaterialInstance
{
    // Albedo/Base Color
    Color AlbedoColor = WHITE;
    std::string AlbedoPath = "";
    bool OverrideAlbedo = false;

    // PBR Maps
    std::string NormalMapPath = "";
    bool OverrideNormal = false;

    std::string MetallicRoughnessPath = "";
    bool OverrideMetallicRoughness = false;

    // Optional Emissive
    std::string EmissivePath = "";
    bool OverrideEmissive = false;

    // Shader
    std::string ShaderPath = "";
    bool OverrideShader = false;

    // Material Parameters
    float Metalness = 0.0f;
    float Roughness = 0.5f;

    // Rendering State
    bool DoubleSided = false;
    bool Transparent = false;
    float Alpha = 1.0f;

    MaterialInstance() = default;
};

BEGIN_REFLECT(MaterialInstance)
    PROPERTY(Color, AlbedoColor, "Albedo Color")
    PROPERTY(std::string, AlbedoPath, "Albedo Path")
    PROPERTY(bool, OverrideAlbedo, "Override Albedo")
    PROPERTY(std::string, NormalMapPath, "Normal Path")
    PROPERTY(bool, OverrideNormal, "Override Normal")
    PROPERTY(std::string, MetallicRoughnessPath, "PBR Path")
    PROPERTY(bool, OverrideMetallicRoughness, "Override PBR")
    PROPERTY(std::string, EmissivePath, "Emissive Path")
    PROPERTY(bool, OverrideEmissive, "Override Emissive")
    PROPERTY(std::string, ShaderPath, "Shader Path")
    PROPERTY(bool, OverrideShader, "Override Shader")
    PROPERTY(float, Metalness, "Metalness")
    PROPERTY(float, Roughness, "Roughness")
    PROPERTY(bool, DoubleSided, "Double Sided")
    PROPERTY(bool, Transparent, "Transparent")
    PROPERTY(float, Alpha, "Alpha")
END_REFLECT()
} // namespace CHEngine

#endif // CH_MATERIAL_H
