#ifndef CH_COMPONENT_UTILS_H
#define CH_COMPONENT_UTILS_H

#include "engine/scene/components/transform_component.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/model_component.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/material_asset.h"
#include "engine/graphics/ui/ui_types.h"
#include <glm/gtx/quaternion.hpp>
#include <filesystem>

namespace Chained::ComponentUtils
{
// --- Transform Logic ---

static inline glm::mat4 GetTransform(const TransformComponent& tc)
{
    return glm::translate(glm::mat4(1.0f), tc.Translation) * glm::toMat4(tc.RotationQuat) *
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

    return glm::translate(glm::mat4(1.0f), interpolatedTranslation) * glm::toMat4(interpolatedRotation) *
           glm::scale(glm::mat4(1.0f), interpolatedScale);
}

// --- UI Layout Logic ---

static inline UIRect CalculateRect(const RectTransform& rt, glm::vec2 viewportSize,
                                   glm::vec2 viewportOffset = {0.0f, 0.0f})
{
    glm::vec2 clAnchMin = glm::clamp(rt.AnchorMin, 0.0f, 1.0f);
    glm::vec2 clAnchMax = glm::clamp(rt.AnchorMax, 0.0f, 1.0f);

    glm::vec2 anchorMinPos = {viewportSize.x * clAnchMin.x, viewportSize.y * clAnchMin.y};
    glm::vec2 anchorMaxPos = {viewportSize.x * clAnchMax.x, viewportSize.y * clAnchMax.y};

    glm::vec2 pMin = {anchorMinPos.x + rt.OffsetMin.x, anchorMinPos.y + rt.OffsetMin.y};
    glm::vec2 pMax = {anchorMaxPos.x + rt.OffsetMax.x, anchorMaxPos.y + rt.OffsetMax.y};

    return UIRect{viewportOffset.x + pMin.x, viewportOffset.y + pMin.y, pMax.x - pMin.x, pMax.y - pMin.y};
}

// --- Mesh logic ---

static inline void ResolveModelPath(ModelComponent& mc)
{
    if (mc.ModelPath.empty())
    {
        mc.ModelHandle = AssetHandle(0);
        return;
    }

    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    auto asset = assets->Get<ModelAsset>(mc.ModelPath);
    if (asset && asset->GetState() == AssetState::Ready)
    {
        mc.ModelHandle = asset->GetID();

        const auto& materials = asset->GetMaterials();
        if (mc.MaterialPaths.empty() || mc.MaterialPaths.size() != materials.size())
        {
            std::filesystem::path modelPath(mc.ModelPath);
            std::string modelName = modelPath.stem().string();
            std::filesystem::path modelDir = modelPath.parent_path();

            mc.MaterialPaths.resize(materials.size());

            for (int i = 0; i < (int)materials.size(); i++)
            {
                std::string matFileName = modelName + "_material_" + std::to_string(i) + ".chmat";
                mc.MaterialPaths[i] = (modelDir / matFileName).generic_string();

                std::string resolvedMatPath = assets->ResolvePath(mc.MaterialPaths[i]);
                if (!std::filesystem::exists(resolvedMatPath))
                {
                    auto matAsset = std::make_shared<MaterialAsset>();
                    matAsset->SetMaterial(materials[i]);
                    matAsset->SaveToFile(resolvedMatPath);
                }
            }
        }
    }
    else
    {
        mc.ModelHandle = AssetHandle(0);
    }
}
} // namespace Chained::ComponentUtils

#endif // CH_COMPONENT_UTILS_H
