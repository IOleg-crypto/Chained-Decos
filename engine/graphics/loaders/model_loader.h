#ifndef CH_MODEL_LOADER_H
#define CH_MODEL_LOADER_H

#include "engine/assets/asset_loader.h"
#include "engine/graphics/assets/model_asset.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

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
    std::shared_ptr<Asset> Create() const override;
    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) override;
    bool IsAsync() const override { return true; }

    static Model GenerateProceduralModel(const std::string& type, const ProceduralParameters& params = ProceduralParameters());
    
    // Finalizes the asset loading (called on the main thread, e.g. for GPU uploads)
    // Returns true if finalization is complete, false if it needs more time in a subsequent frame.
    static bool Finalize(std::shared_ptr<ModelAsset> asset, std::chrono::steady_clock::time_point budgetEnd);

private:
    static PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS = 30);
};
} // namespace CHEngine

#endif // CH_MODEL_LOADER_H
