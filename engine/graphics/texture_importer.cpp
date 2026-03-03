#include "texture_importer.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<TextureAsset> TextureImporter::ImportTexture(const std::filesystem::path& path)
{
    CH_CORE_INFO("TextureImporter: Registering texture path: {}", path.string());

    auto asset = std::make_shared<TextureAsset>();
    asset->SetPath(path.string());
    asset->SetState(AssetState::Ready);
    return asset;
}

Image TextureImporter::LoadImageFromDisk(const std::filesystem::path& path)
{
    return LoadImage(path.string().c_str());
}
} // namespace CHEngine
