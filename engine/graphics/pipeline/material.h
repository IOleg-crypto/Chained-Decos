#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

#include "asset.h"
#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
// Editable material parameters and asset references used by the renderer and inspector.
struct MaterialInstance
{
    // Albedo/Base Color
    Color AlbedoColor = {255, 255, 255, 255};
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
    Color EmissiveColor = {0, 0, 0, 255};
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
};

CH_MARK_RFL(MaterialInstance);

} // namespace Chained

#endif // CH_MATERIAL_H
