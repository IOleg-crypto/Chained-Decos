#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "asset.h"
#include "engine/core/base.h"
#include "model_data.h"
#include <future>
#include "raylib.h"
#include <string>
#include <vector>
#include <mutex>

namespace CHEngine
{
class ModelAsset : public Asset
{
public:
    ModelAsset() : Asset(GetStaticType()) {}
    virtual ~ModelAsset();

    static AssetType GetStaticType() { return AssetType::Model; }

    void UploadToGPU(); // Main thread
    
    // For internal use by MeshImporter
    void SetPendingData(const PendingModelData& data) { m_PendingData = data; m_HasPendingData = true; }

    Model &GetModel();
    const Model &GetModel() const;

    BoundingBox GetBoundingBox() const;

    void OnUpdate(); // Check if textures loaded and apply them
    
    void UpdateAnimation(int animationIndex, int frame);
    ModelAnimation *GetAnimations(int *count);
    int GetAnimationCount() const;

    const std::vector<std::shared_ptr<class TextureAsset>> &GetTextures() const;

private:
    Model m_Model = {0};
    ModelAnimation *m_Animations = nullptr;
    int m_AnimationCount = 0;
    std::vector<std::shared_ptr<class TextureAsset>> m_Textures;
    
    mutable std::mutex m_ModelMutex;  // Protect Model access during async loading
    
    PendingModelData m_PendingData;
    bool m_HasPendingData = false;
    
    // Track textures that are still loading
    std::vector<PendingTexture> m_PendingTextures;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
