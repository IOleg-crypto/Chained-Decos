#include "font_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"

namespace CHEngine
{
FontAsset::~FontAsset()
{
    if (m_Font.texture.id > 0)
        UnloadFont(m_Font);
}

std::shared_ptr<FontAsset> FontAsset::Load(const std::string &path)
{
    std::filesystem::path fullPath = AssetManager::ResolvePath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("FontAsset: File not found: {0}", fullPath.string());
        return nullptr;
    }

    auto asset = std::make_shared<FontAsset>();
    asset->SetPath(path);

    // Raylib font loading
    asset->m_Font = LoadFont(fullPath.string().c_str());

    if (asset->m_Font.texture.id == 0)
    {
        CH_CORE_ERROR("FontAsset: Failed to load font: {0}", path);
        return nullptr;
    }

    return asset;
}
} // namespace CHEngine
