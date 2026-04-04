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
        if (props.BeginGroup("Albedo"))
        {
            props.Property("Color", AlbedoColor);
            props.File("Path", AlbedoPath, "png,jpg,tga");
            props.Property("Override", OverrideAlbedo);
            props.EndGroup();
        }

        if (props.BeginGroup("PBR Maps"))
        {
            props.File("Normal Map", NormalMapPath, "png,jpg,tga");
            props.Property("Override Normal", OverrideNormal);
            props.File("MR Map", MetallicRoughnessPath, "png,jpg,tga");
            props.Property("Override MR", OverrideMetallicRoughness);
            props.File("Occlusion Map", OcclusionMapPath, "png,jpg,tga");
            props.Property("Override Occlusion", OverrideOcclusion);
            props.EndGroup();
        }

        if (props.BeginGroup("Emissive"))
        {
            props.Property("Color", EmissiveColor);
            props.Property("Intensity", EmissiveIntensity);
            props.File("Path", EmissivePath, "png,jpg,tga");
            props.Property("Override", OverrideEmissive);
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
            props.Property("Double Sided", DoubleSided);
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
