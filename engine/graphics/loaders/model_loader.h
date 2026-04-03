#ifndef CH_MODEL_LOADER_H
#define CH_MODEL_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/model_asset.h"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

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
};
} // namespace CHEngine

#endif // CH_MODEL_LOADER_H
