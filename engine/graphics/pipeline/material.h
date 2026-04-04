#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

#include "engine/core/base.h"
#include "engine/core/reflection.h"
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

    CH_REFLECT_BEGIN(MaterialInstance)
        props.Property("Albedo Color", AlbedoColor);
        props.File("Albedo Path", AlbedoPath, "png,jpg,tga");
        props.Property("Override Albedo", OverrideAlbedo);
        props.File("Normal Path", NormalMapPath, "png,jpg,tga");
        props.Property("Override Normal", OverrideNormal);
        props.File("MR Path", MetallicRoughnessPath, "png,jpg,tga");
        props.Property("Override MR", OverrideMetallicRoughness);
        props.File("Occlusion Path", OcclusionMapPath, "png,jpg,tga");
        props.Property("Override Occlusion", OverrideOcclusion);
        props.File("Emissive Path", EmissivePath, "png,jpg,tga");
        props.Property("Emissive Color", EmissiveColor);
        props.Property("Emissive Intensity", EmissiveIntensity);
        props.Property("Override Emissive", OverrideEmissive);
        props.File("Shader Path", ShaderPath, "chshader");
        props.Property("Override Shader", OverrideShader);
        props.Property("Metalness", Metalness);
        props.Property("Roughness", Roughness);
        props.Property("Double Sided", DoubleSided);
        props.Property("Transparent", Transparent);
        props.Property("Alpha", Alpha);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_MATERIAL_H
