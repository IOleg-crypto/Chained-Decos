#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

#include "engine/core/reflection.h"

namespace CHEngine
{
// Editable material parameters and asset references used by the renderer and inspector.
struct MaterialInstance
{
    // Albedo/Base Color
    Color AlbedoColor = { 255, 255, 255, 255 };
    AssetHandle AlbedoHandle = AssetHandle(0);
    bool OverrideAlbedo = false;

    // PBR Maps
    AssetHandle NormalHandle = AssetHandle(0);
    bool OverrideNormal = false;

    AssetHandle MetallicRoughnessHandle = AssetHandle(0);
    bool OverrideMetallicRoughness = false;

    AssetHandle OcclusionHandle = AssetHandle(0);
    bool OverrideOcclusion = false;

    // Optional Emissive
    AssetHandle EmissiveHandle = AssetHandle(0);
    Color EmissiveColor = { 0, 0, 0, 255 };
    float EmissiveIntensity = 0.0f;
    bool OverrideEmissive = false;

    // Shader
    AssetHandle ShaderHandle = AssetHandle(0);
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
            props.Handle("Albedo", AlbedoHandle);
            props.Property("OverrideAlbedo", OverrideAlbedo);
            props.EndGroup();
        }

        if (props.BeginGroup("PBR Maps"))
        {
            props.Handle("Normal", NormalHandle);
            props.Property("OverrideNormal", OverrideNormal);
            props.Handle("MetallicRoughness", MetallicRoughnessHandle);
            props.Property("OverrideMetallicRoughness", OverrideMetallicRoughness);
            props.Handle("Occlusion", OcclusionHandle);
            props.Property("OverrideOcclusion", OverrideOcclusion);
            props.EndGroup();
        }

        if (props.BeginGroup("Emissive"))
        {
            props.Property("EmissiveColor", EmissiveColor);
            props.Property("EmissiveIntensity", EmissiveIntensity);
            props.Handle("Emissive", EmissiveHandle);
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
            props.Handle("Shader", ShaderHandle);
            props.Property("Override", OverrideShader);
            props.EndGroup();
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_MATERIAL_H
