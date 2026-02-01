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

    MaterialInstance() = default;
};
} // namespace CHEngine

#endif // CH_MATERIAL_H
