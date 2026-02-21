#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "asset.h"
#include "engine/core/base.h"
#include "model_data.h"
#include "raylib.h"
#include <future>
#include <mutex>
#include <string>
#include <vector>

namespace CHEngine
{
class ModelAsset : public Asset
{
public:
    ModelAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~ModelAsset();

    static AssetType GetStaticType()
    {
        return AssetType::Model;
    }

    void UploadToGPU(); // Main thread

    // For internal use by MeshImporter
    void SetPendingData(const PendingModelData& data)
    {
        m_PendingData = data;
        m_HasPendingData = true;
    }

    Model& GetModel();
    const Model& GetModel() const;

    BoundingBox GetBoundingBox() const;

    void OnUpdate(); // Check if textures loaded and apply them

    void UpdateAnimation(int animationIndex, int frame);
    const std::vector<RawAnimation>& GetRawAnimations() const
    {
        return m_Animations;
    }
    int GetAnimationCount() const
    {
        return (int)m_Animations.size();
    }
    std::string GetAnimationName(int index) const
    {
        return (index >= 0 && index < (int)m_Animations.size()) ? m_Animations[index].name : "";
    }

    std::vector<std::shared_ptr<class TextureAsset>> GetTextures() const;
    const std::vector<Matrix>& GetOffsetMatrices() const
    {
        return m_OffsetMatrices;
    }
    const std::vector<int>& GetMeshToNode() const
    {
        return m_MeshToNode;
    }
    const std::vector<Matrix>& GetGlobalNodeTransforms() const
    {
        return m_GlobalNodeTransforms;
    }

private:
    Model m_Model = {0};
    std::vector<RawAnimation> m_Animations;
    std::vector<std::shared_ptr<class TextureAsset>> m_Textures;

    // KISS additions
    std::vector<Matrix> m_OffsetMatrices;
    std::vector<std::string> m_NodeNames;
    std::vector<int> m_NodeParents;
    std::vector<int> m_MeshToNode;
    std::vector<Matrix> m_GlobalNodeTransforms;

    mutable std::mutex m_ModelMutex; // Protect Model access during async loading

    PendingModelData m_PendingData;
    bool m_HasPendingData = false;

    // Track textures that are still loading
    std::vector<PendingTexture> m_PendingTextures;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
