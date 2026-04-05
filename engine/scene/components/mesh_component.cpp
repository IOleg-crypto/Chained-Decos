#include "engine/scene/components/mesh_component.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"

namespace CHEngine {

void ModelComponent::SyncMaterials(AssetHandle handle)
{
    if (MaterialsInitialized || handle == 0) return;
    
    auto model = AssetManager::Get().Get<ModelAsset>(handle);
    if (!model || !model->IsReady()) return;

    auto& modelMaterials = model->GetModel().Materials;
    Materials.clear();
    for (size_t i = 0; i < modelMaterials.size(); i++)
    {
        const auto& mat = modelMaterials[i];
        MaterialSlot slot;
        slot.Name = mat.Name;
        slot.Index = (int)i;
        slot.Target = MaterialSlotTarget::MaterialIndex;
        
        // Convert Material (float colors) to MaterialInstance (byte colors)
        slot.Material.AlbedoColor = { 
            (unsigned char)glm::clamp(mat.AlbedoColor.r * 255.0f, 0.0f, 255.0f),
            (unsigned char)glm::clamp(mat.AlbedoColor.g * 255.0f, 0.0f, 255.0f),
            (unsigned char)glm::clamp(mat.AlbedoColor.b * 255.0f, 0.0f, 255.0f),
            (unsigned char)glm::clamp(mat.AlbedoColor.a * 255.0f, 0.0f, 255.0f)
        };
        slot.Material.AlbedoPath = mat.AlbedoPath;
        slot.Material.OverrideAlbedo = !mat.AlbedoPath.empty();
        
        slot.Material.NormalMapPath = mat.NormalPath;
        slot.Material.OverrideNormal = !mat.NormalPath.empty();
        
        slot.Material.MetallicRoughnessPath = mat.MetallicRoughnessPath;
        slot.Material.OverrideMetallicRoughness = !mat.MetallicRoughnessPath.empty();
        
        slot.Material.OcclusionMapPath = mat.OcclusionPath;
        slot.Material.OverrideOcclusion = !mat.OcclusionPath.empty();
        
        slot.Material.EmissivePath = mat.EmissivePath;
        slot.Material.OverrideEmissive = !mat.EmissivePath.empty();
        
        slot.Material.EmissiveColor = {
            (unsigned char)glm::clamp(mat.EmissiveColor.r * 255.0f, 0.0f, 255.0f),
            (unsigned char)glm::clamp(mat.EmissiveColor.g * 255.0f, 0.0f, 255.0f),
            (unsigned char)glm::clamp(mat.EmissiveColor.b * 255.0f, 0.0f, 255.0f),
            (unsigned char)glm::clamp(mat.EmissiveColor.a * 255.0f, 0.0f, 255.0f)
        };
        slot.Material.EmissiveIntensity = mat.EmissiveIntensity;
        slot.Material.Metalness = mat.Metalness;
        slot.Material.Roughness = mat.Roughness;
        
        Materials.push_back(slot);
    }
    MaterialsInitialized = true;
}

}
