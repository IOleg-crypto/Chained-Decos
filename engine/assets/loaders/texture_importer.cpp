#include "asset_importer.h"
#include "engine/assets/asset_manager.h"
#include "engine/graphics/api/texture.h"

namespace Chained::AssetImporter
{
std::shared_ptr<TextureAsset> ImportTexture(AssetHandle handle, const AssetMetadata& metadata)
{
    std::filesystem::path fullPath = AssetManager::Get().GetAssetDirectory() / metadata.FilePath;
    std::string pathString = fullPath.string();

    // Use the graphics API factory to handle actual disk load logic (which may contain stb_image)
    std::shared_ptr<Texture> gpuTexture = Texture::CreateFromFile(pathString);

    if (!gpuTexture)
    {
        return nullptr;
    }

    std::shared_ptr<TextureAsset> asset = std::make_shared<TextureAsset>(handle, gpuTexture);
    asset->SetPath(metadata.FilePath.string());

    return asset;
}
} // namespace Chained::AssetImporter
