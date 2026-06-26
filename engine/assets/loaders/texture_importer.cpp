#include "asset_importer.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"

#include <stb_image.h>
#include <filesystem>

namespace Chained::AssetImporter
{

// CPU-only decode. SAFE to call from any thread — no OpenGL.
// GPU upload is deferred to main thread via TextureAsset::Finalize()
// called from AssetManager::Update().
std::shared_ptr<TextureAsset> ImportTexture(AssetHandle handle, const AssetMetadata& metadata)
{
    auto* am = ServiceLocator::Get<AssetManager>();

    std::filesystem::path fullPath = metadata.FilePath;
    if (fullPath.is_relative() && am)
        fullPath = am->GetAssetDirectory() / fullPath;

    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_WARN("TextureImporter: File not found: {}", fullPath.string());
        return nullptr;
    }

    const std::string pathStr = fullPath.string();
    bool isHDR = stbi_is_hdr(pathStr.c_str());
    stbi_set_flip_vertically_on_load(!isHDR);

    int width = 0, height = 0, channels = 0;
    void* data = nullptr;

    if (isHDR)
        data = stbi_loadf(pathStr.c_str(), &width, &height, &channels, 0);
    else
    {
        data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
        channels = 4;
    }

    if (!data)
    {
        CH_CORE_ERROR("TextureImporter: stbi failed for '{}': {}", pathStr, stbi_failure_reason());
        return nullptr;
    }

    // Create asset and store raw CPU data. GPU upload happens in AssetManager::Update().
    auto asset = std::make_shared<TextureAsset>(handle);
    asset->SetIsHDR(isHDR);
    asset->SetRawData(data, width, height, channels, isHDR);
    asset->SetPath(metadata.FilePath.string());
    asset->SetState(AssetState::Loading); // Not Ready yet — pending GPU finalization

    return asset;
}

} // namespace Chained::AssetImporter
