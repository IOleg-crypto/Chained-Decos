#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "asset.h"
#include "engine/core/base.h"
#include <future>
#include "raylib.h"
#include <string>
#include <vector>

namespace CHEngine
{
class ModelAsset : public Asset
{
public:
    static std::shared_ptr<ModelAsset> Load(const std::string &path);
    static void LoadAsync(const std::string &path);
    static std::shared_ptr<ModelAsset> CreateProcedural(const std::string &type);

    ModelAsset() = default;
    virtual ~ModelAsset();

    virtual AssetType GetType() const override
    {
        return AssetType::Model;
    }

    void UploadToGPU(); // Main thread
    void LoadFromFile(const std::string &path);
    Model &GetModel()
    {
        return m_Model;
    }
    const Model &GetModel() const
    {
        return m_Model;
    }

    BoundingBox GetBoundingBox() const;

    void UpdateAnimation(int animIndex, int frame);
    ModelAnimation *GetAnimations(int *count);
    int GetAnimationCount() const
    {
        return m_AnimCount;
    }

    const std::vector<std::shared_ptr<class TextureAsset>> &GetTextures() const
    {
        return m_Textures;
    }

    std::shared_ptr<class BVH> GetBVHCache()
    {
        if (!m_BVHCache && m_BVHFuture.valid())
        {
            if (m_BVHFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                m_BVHCache = m_BVHFuture.get();
            }
        }
        return m_BVHCache;
    }
    void SetBVHCache(std::shared_ptr<BVH> bvh)
    {
        m_BVHCache = bvh;
    }

private:
    Model m_Model = {0};
    ModelAnimation *m_Animations = nullptr;
    int m_AnimCount = 0;
    std::vector<std::shared_ptr<class TextureAsset>> m_Textures;
    std::shared_ptr<class BVH> m_BVHCache;
    std::shared_future<std::shared_ptr<class BVH>> m_BVHFuture;
    
    // Deferred loading: CPU work done in background, GPU upload on main thread
    Mesh m_PendingMesh = {0};
    bool m_HasPendingMesh = false;
    std::string m_PendingModelPath;
    bool m_HasPendingModel = false;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
