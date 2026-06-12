#include "asset_importer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/loaders/assimp_importer.h"

namespace Chained::AssetImporter
{
std::shared_ptr<ModelAsset> ImportModel(AssetHandle handle, const AssetMetadata& metadata)
{
    std::filesystem::path fullPath = AssetManager::Get().GetAssetDirectory() / metadata.FilePath;

    // Use AssimpImporter to retrieve the data
    PendingModelData pendingData = AssimpImporter::Import(fullPath);

    // If the import failed, it usually results in an empty model structure, check validity here
    // if (!pendingData.IsValid()) return nullptr;

    std::shared_ptr<ModelAsset> asset = std::make_shared<ModelAsset>(handle);
    asset->SetPath(metadata.FilePath.string());
    asset->m_PendingData = std::move(pendingData);
    asset->m_HasPendingData = true;

    // Fire OnLoaded manually since it's no longer automatic via ThreadPool
    asset->OnLoaded();
    asset->SetState(AssetState::Ready);

    return asset;
}
} // namespace Chained::AssetImporter
