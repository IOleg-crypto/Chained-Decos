#ifndef CH_ASSET_IMPORTER_H
#define CH_ASSET_IMPORTER_H

#include "engine/assets/asset_metadata.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/font_asset.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include <memory>

namespace Chained::AssetImporter
{
std::shared_ptr<TextureAsset> ImportTexture(AssetHandle handle, const AssetMetadata& metadata);
std::shared_ptr<ModelAsset> ImportModel(AssetHandle handle, const AssetMetadata& metadata);
std::shared_ptr<FontAsset> ImportFont(AssetHandle handle, const AssetMetadata& metadata);
std::shared_ptr<ShaderAsset> ImportShader(AssetHandle handle, const AssetMetadata& metadata);
std::shared_ptr<EnvironmentAsset> ImportEnvironment(AssetHandle handle, const AssetMetadata& metadata);
} // namespace Chained::AssetImporter

#endif // CH_ASSET_IMPORTER_H
