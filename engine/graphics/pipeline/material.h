#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

#include "engine/core/base.h"
#include <string>

namespace CHEngine
{
struct MaterialInstance
{
    // Albedo/Base Color
    Color AlbedoColor = { 255, 255, 255, 255 };
    std::string AlbedoPath = "";
    bool OverrideAlbedo = false;

    // PBR Maps
    std::string NormalMapPath = "";
    bool OverrideNormal = false;

    std::string MetallicRoughnessPath = "";
    bool OverrideMetallicRoughness = false;

    std::string OcclusionMapPath = "";
    bool OverrideOcclusion = false;

    // Optional Emissive
    std::string EmissivePath = "";
    Color EmissiveColor = { 0, 0, 0, 255 };
    float EmissiveIntensity = 0.0f;
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

} // namespace CHEngine

#endif // CH_MATERIAL_H
