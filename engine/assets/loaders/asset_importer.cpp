#include "asset_importer.h"
#include "engine/core/log.h"

namespace Chained::AssetImporter
{
std::shared_ptr<TextureAsset> ImportTexture(AssetHandle handle, const AssetMetadata& metadata)
{
    CH_CORE_WARN("[AssetImporter] Stub: ImportTexture called for '{}'", metadata.FilePath.string());
    return nullptr;
}

std::shared_ptr<ModelAsset> ImportModel(AssetHandle handle, const AssetMetadata& metadata)
{
    CH_CORE_WARN("[AssetImporter] Stub: ImportModel called for '{}'", metadata.FilePath.string());
    return nullptr;
}

std::shared_ptr<FontAsset> ImportFont(AssetHandle handle, const AssetMetadata& metadata)
{
    CH_CORE_WARN("[AssetImporter] Stub: ImportFont called for '{}'", metadata.FilePath.string());
    return nullptr;
}

std::shared_ptr<ShaderAsset> ImportShader(AssetHandle handle, const AssetMetadata& metadata)
{
    CH_CORE_WARN("[AssetImporter] Stub: ImportShader called for '{}'", metadata.FilePath.string());
    return nullptr;
}

std::shared_ptr<EnvironmentAsset> ImportEnvironment(AssetHandle handle, const AssetMetadata& metadata)
{
    CH_CORE_WARN("[AssetImporter] Stub: ImportEnvironment called for '{}'", metadata.FilePath.string());
    return nullptr;
}
} // namespace Chained::AssetImporter
