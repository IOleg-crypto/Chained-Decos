#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "asset.h"
#include "engine/core/base.h"
#include <raylib.h>
#include <string>
#include <vector>


namespace CHEngine
{
class ModelAsset : public Asset
{
public:
    static Ref<ModelAsset> Load(const std::string &path);
    static Ref<ModelAsset> CreateProcedural(const std::string &type);

    ModelAsset() = default;
    virtual ~ModelAsset();

    virtual AssetType GetType() const override
    {
        return AssetType::Model;
    }

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

    const std::vector<Ref<class TextureAsset>> &GetTextures() const
    {
        return m_Textures;
    }

    Ref<class BVHNode> GetBVHCache() const
    {
        return m_BVHCache;
    }
    void SetBVHCache(Ref<class BVHNode> bvh)
    {
        m_BVHCache = bvh;
    }

private:
    Model m_Model = {0};
    ModelAnimation *m_Animations = nullptr;
    int m_AnimCount = 0;
    std::vector<Ref<class TextureAsset>> m_Textures;
    Ref<class BVHNode> m_BVHCache;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
