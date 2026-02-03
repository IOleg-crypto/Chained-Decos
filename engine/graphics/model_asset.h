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
    static Model GenerateProceduralModel(const std::string &type);

    ModelAsset() = default;
    virtual ~ModelAsset();

    virtual AssetType GetType() const override;

    void UploadToGPU(); // Main thread
    void LoadFromFile(const std::string &path);
    Model &GetModel();
    const Model &GetModel() const;

    BoundingBox GetBoundingBox() const;

    void UpdateAnimation(int animIndex, int frame);
    ModelAnimation *GetAnimations(int *count);
    int GetAnimationCount() const;

    const std::vector<std::shared_ptr<class TextureAsset>> &GetTextures() const;


private:
    Model m_Model = {0};
    ModelAnimation *m_Animations = nullptr;
    int m_AnimCount = 0;
    std::vector<std::shared_ptr<class TextureAsset>> m_Textures;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
