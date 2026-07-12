#ifndef CH_MODEL_LOADER_H
#define CH_MODEL_LOADER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/types/model_asset.h"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace Chained
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

struct ChainedAssetHeader
{
    uint32_t magic = 0x43484153; 
    uint32_t version = 2; 
    uint32_t dataStructSize = sizeof(PendingModelData);
    uint64_t sourceHash = 0;
    bool compressed = false;
    uint64_t compressedSize = 0;
    uint64_t uncompressedSize = 0;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(magic, version, dataStructSize, sourceHash, compressed, compressedSize, uncompressedSize);
    }
};

namespace ModelLoader
{
    std::shared_ptr<Asset> Create();
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);

    Model GenerateProceduralModel(const std::string& type, const ProceduralParameters& params = ProceduralParameters());

    PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS = 30);
} // namespace ModelLoader
} // namespace Chained

#endif // CH_MODEL_LOADER_H