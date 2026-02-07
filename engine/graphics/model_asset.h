#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "asset.h"
#include "engine/core/base.h"
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

    void OnUpdate(); // Check if textures loaded and apply them
    
    void UpdateAnimation(int animationIndex, int frame);
    ModelAnimation *GetAnimations(int *count);
    int GetAnimationCount() const;

    const std::vector<std::shared_ptr<class TextureAsset>> &GetTextures() const;

    struct RawMesh {
        std::vector<float> vertices;
        std::vector<float> texcoords;
        std::vector<float> normals;
        std::vector<unsigned char> colors;
        std::vector<unsigned short> indices;
        int materialIndex = -1;
    };

    struct RawMaterial {
        std::string albedoPath;
        Color albedoColor = WHITE;
    };

    // CPU-side data for async loading (loaded in worker thread)
    struct PendingModelData {
        std::string fullPath;   
        std::vector<RawMesh> meshes;
        std::vector<RawMaterial> materials;
        
        ModelAnimation* animations = nullptr;
        int animationCount = 0;
        bool isValid = false;
    };

private:
    Model m_Model = {0};
    ModelAnimation *m_Animations = nullptr;
    int m_AnimationCount = 0;
    std::vector<std::shared_ptr<class TextureAsset>> m_Textures;
    
    mutable std::mutex m_ModelMutex;  // Protect Model access during async loading
    
    PendingModelData m_PendingData;
    bool m_HasPendingData = false;
    
    // Track textures that are still loading
    struct PendingTexture {
        int materialIndex;
        std::string path;
    };
    std::vector<PendingTexture> m_PendingTextures;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
