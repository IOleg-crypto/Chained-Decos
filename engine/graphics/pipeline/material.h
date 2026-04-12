#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

#include "engine/core/base.h"
#include "engine/core/reflection.h"
#include <string>

namespace CHEngine
{
// Editable material parameters and asset references used by the renderer and inspector.
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
        if (props.BeginGroup("Albedo"))
        {
            props.Property("AlbedoColor", AlbedoColor);
            props.File("AlbedoPath", AlbedoPath, "png,jpg,tga");
            props.Property("OverrideAlbedo", OverrideAlbedo);
            props.EndGroup();
        }

        if (props.BeginGroup("PBR Maps"))
        {
            props.File("NormalMapPath", NormalMapPath, "png,jpg,tga");
            props.Property("OverrideNormal", OverrideNormal);
            props.File("MetallicRoughnessPath", MetallicRoughnessPath, "png,jpg,tga");
            props.Property("OverrideMetallicRoughness", OverrideMetallicRoughness);
            props.File("OcclusionMapPath", OcclusionMapPath, "png,jpg,tga");
            props.Property("OverrideOcclusion", OverrideOcclusion);
            props.EndGroup();
        }

        if (props.BeginGroup("Emissive"))
        {
            props.Property("EmissiveColor", EmissiveColor);
            props.Property("EmissiveIntensity", EmissiveIntensity);
            props.File("EmissivePath", EmissivePath, "png,jpg,tga");
            props.Property("OverrideEmissive", OverrideEmissive);
            props.EndGroup();
        }

        if (props.BeginGroup("Parameters"))
        {
            props.Property("Metalness", Metalness);
            props.Property("Roughness", Roughness);
            props.EndGroup();
        }

        if (props.BeginGroup("Rendering"))
        {
            props.Property("DoubleSided", DoubleSided);
            props.Property("Transparent", Transparent);
            if (Transparent)
                props.Property("Alpha", Alpha);
            props.EndGroup();
        }

        if (props.BeginGroup("Shader"))
        {
            props.File("Path", ShaderPath, "chshader");
            props.Property("Override", OverrideShader);
            props.EndGroup();
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_MATERIAL_H
