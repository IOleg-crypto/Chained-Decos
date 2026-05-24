#ifndef CH_COMPONENT_UTILS_H
#define CH_COMPONENT_UTILS_H

#include "engine/scene/components/transform_component.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include <glm/gtx/quaternion.hpp>

namespace CHEngine::ComponentUtils
{
    // --- Transform Logic ---

    static inline glm::mat4 GetTransform(const TransformComponent& tc)
    {
        return glm::translate(glm::mat4(1.0f), tc.Translation) * 
               glm::toMat4(tc.RotationQuat) * 
               glm::scale(glm::mat4(1.0f), tc.Scale);
    }

    static inline void SetTranslation(TransformComponent& tc, const glm::vec3& translation)
    {
        tc.Translation = translation;
        tc.IsDirty = true;
    }

    static inline void SetRotation(TransformComponent& tc, const glm::vec3& eulerAngles)
    {
        tc.Rotation = eulerAngles;
        tc.RotationQuat = glm::quat(eulerAngles);
        tc.IsDirty = true;
    }

    static inline void SetScale(TransformComponent& tc, const glm::vec3& scale)
    {
        tc.Scale = scale;
        tc.IsDirty = true;
    }

    static inline void SetRotationQuat(TransformComponent& tc, const glm::quat& rotationQuat)
    {
        tc.RotationQuat = rotationQuat;
        tc.Rotation = glm::eulerAngles(rotationQuat);
        tc.IsDirty = true;
    }

    static inline glm::mat4 GetInterpolatedTransform(const TransformComponent& tc, float alpha)
    {
        glm::vec3 interpolatedTranslation = glm::mix(tc.PrevTranslation, tc.Translation, alpha);
        glm::quat interpolatedRotation = glm::slerp(tc.PrevRotationQuat, tc.RotationQuat, alpha);
        glm::vec3 interpolatedScale = glm::mix(tc.PrevScale, tc.Scale, alpha);

        return glm::translate(glm::mat4(1.0f), interpolatedTranslation) * 
               glm::toMat4(interpolatedRotation) * 
               glm::scale(glm::mat4(1.0f), interpolatedScale);
    }

    // --- UI Layout Logic ---

    static inline Rectangle CalculateRect(const RectTransform& rt, glm::vec2 viewportSize, glm::vec2 viewportOffset = {0.0f, 0.0f})
    {
        glm::vec2 clAnchMin = glm::clamp(rt.AnchorMin, 0.0f, 1.0f);
        glm::vec2 clAnchMax = glm::clamp(rt.AnchorMax, 0.0f, 1.0f);

        glm::vec2 anchorMinPos = {viewportSize.x * clAnchMin.x, viewportSize.y * clAnchMin.y};
        glm::vec2 anchorMaxPos = {viewportSize.x * clAnchMax.x, viewportSize.y * clAnchMax.y};

        glm::vec2 pMin = {anchorMinPos.x + rt.OffsetMin.x, anchorMinPos.y + rt.OffsetMin.y};
        glm::vec2 pMax = {anchorMaxPos.x + rt.OffsetMax.x, anchorMaxPos.y + rt.OffsetMax.y};

        return Rectangle{viewportOffset.x + pMin.x, viewportOffset.y + pMin.y, pMax.x - pMin.x, pMax.y - pMin.y};
    }

    // --- Mesh & Material Logic ---

    static inline void SyncMaterials(ModelComponent& mc, AssetHandle handle, AssetManager* assetManager = nullptr)
    {
        if (mc.MaterialsInitialized || handle == 0) return;
        
        if (!assetManager && ServiceLocator::Has<AssetManager>())
            assetManager = &ServiceLocator::Get<AssetManager>();

        auto model = assetManager ? assetManager->Get<ModelAsset>(handle) : nullptr;
        if (!model || !model->IsReady()) return;

        auto& modelMaterials = model->GetModel().Materials;
        mc.Materials.clear();
        for (size_t i = 0; i < modelMaterials.size(); i++)
        {
            const auto& mat = modelMaterials[i];
            MaterialSlot slot;
            slot.Name = mat.Name;
            slot.Index = (int)i;
            slot.Target = MaterialSlotTarget::MaterialIndex;
            
            slot.Material.AlbedoColor = { 
                (unsigned char)glm::clamp(mat.AlbedoColor.r * 255.0f, 0.0f, 255.0f),
                (unsigned char)glm::clamp(mat.AlbedoColor.g * 255.0f, 0.0f, 255.0f),
                (unsigned char)glm::clamp(mat.AlbedoColor.b * 255.0f, 0.0f, 255.0f),
                (unsigned char)glm::clamp(mat.AlbedoColor.a * 255.0f, 0.0f, 255.0f)
            };
            slot.Material.AlbedoHandle = mat.AlbedoHandle;
            slot.Material.OverrideAlbedo = (mat.AlbedoHandle != AssetHandle(0));
            slot.Material.NormalHandle = mat.NormalHandle;
            slot.Material.OverrideNormal = (mat.NormalHandle != AssetHandle(0));
            slot.Material.MetallicRoughnessHandle = mat.MetallicRoughnessHandle;
            slot.Material.OverrideMetallicRoughness = (mat.MetallicRoughnessHandle != AssetHandle(0));
            slot.Material.OcclusionHandle = mat.OcclusionHandle;
            slot.Material.OverrideOcclusion = (mat.OcclusionHandle != AssetHandle(0));
            slot.Material.EmissiveHandle = mat.EmissiveHandle;
            slot.Material.OverrideEmissive = (mat.EmissiveHandle != AssetHandle(0));
            slot.Material.EmissiveColor = {
                (unsigned char)glm::clamp(mat.EmissiveColor.r * 255.0f, 0.0f, 255.0f),
                (unsigned char)glm::clamp(mat.EmissiveColor.g * 255.0f, 0.0f, 255.0f),
                (unsigned char)glm::clamp(mat.EmissiveColor.b * 255.0f, 0.0f, 255.0f),
                (unsigned char)glm::clamp(mat.EmissiveColor.a * 255.0f, 0.0f, 255.0f)
            };
            slot.Material.EmissiveIntensity = mat.EmissiveIntensity;
            slot.Material.Metalness = mat.Metalness;
            slot.Material.Roughness = mat.Roughness;
            
            mc.Materials.push_back(slot);
        }
        mc.MaterialsInitialized = true;
    }

    static inline void ResolveModelPath(ModelComponent& mc)
    {
        if (mc.ModelPath.empty())
        {
            mc.ModelHandle = AssetHandle(0);
            return;
        }

        AssetManager* assetManager = nullptr;
        if (ServiceLocator::Has<AssetManager>())
            assetManager = &ServiceLocator::Get<AssetManager>();

        auto handle = assetManager ? assetManager->ResolveToHandle(mc.ModelPath, ModelAsset::GetStaticType()) : AssetHandle(0);
        auto asset = assetManager ? assetManager->Get<ModelAsset>(handle) : nullptr;
        if (asset)
        {
            AssetHandle newHandle = asset->GetID();
            
            // Only sync materials if the model handle changed OR if materials were never initialized.
            // We also check if mc.Materials is empty to handle potentially stale states after scene duplication.
            if (newHandle != mc.ModelHandle || !mc.MaterialsInitialized || mc.Materials.empty())
            {
                bool isDeserializingWithMaterials = (mc.ModelHandle == AssetHandle(0) && !mc.Materials.empty());
                
                mc.ModelHandle = newHandle;
                mc.MaterialsInitialized = false; 
                
                if (isDeserializingWithMaterials)
                {
                    // Booting from serialized scene where materials were already loaded from JSON
                    mc.MaterialsInitialized = true;
                }
                else
                {
                    // Actual model swap or brand new model, parse default materials from FBX
                    SyncMaterials(mc, mc.ModelHandle);
                }
            }
        }
        else
        {
            mc.ModelHandle = AssetHandle(0);
            // We don't reset MaterialsInitialized here because the asset might just be currently loading.
        }
    }
}

#endif // CH_COMPONENT_UTILS_H
