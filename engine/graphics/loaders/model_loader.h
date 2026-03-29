#ifndef CH_MODEL_LOADER_H
#define CH_MODEL_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/model_asset.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <assimp/scene.h>

namespace CHEngine
{
struct ProceduralParameters
{
    float Radius = 0.5f;
    float InnerRadius = 0.2f;
    float Height = 1.0f;
    int Slices = 16;
    int Stacks = 16;
    glm::vec3 Dimensions = {1.0f, 1.0f, 1.0f};
};

class ModelLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override;
    bool IsAsync() const override { return true; }

    static Model GenerateProceduralModel(const std::string& type, const ProceduralParameters& params = ProceduralParameters());

private:
    static PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS = 30);
    
    // Assimp processing helpers
    static void ProcessHierarchy(aiNode* node, int parent, PendingModelData& data, std::map<aiNode*, int>& nodeToBone);
    static void ProcessMaterials(const aiScene* scene, const std::filesystem::path& modelDir, PendingModelData& data);
    static void ProcessMeshes(const aiScene* scene, PendingModelData& data);
    static void BuildSkeleton(PendingModelData& data);
    static void ProcessAnimations(const aiScene* scene, PendingModelData& data, int samplingFPS);
    static std::string ResolveTexturePath(const aiScene* scene, const aiMaterial* aiMat, aiTextureType type, const std::filesystem::path& modelDir);

    // Conversion helpers
    static glm::mat4 ConvertMatrix(const aiMatrix4x4& m);
    static glm::vec3 ConvertVector3(const aiVector3D& v);
    static glm::quat ConvertQuaternion(const aiQuaternion& q);
    static glm::vec4 ConvertColor(const aiColor4D& c);
};
} // namespace CHEngine

#endif // CH_MODEL_LOADER_H
