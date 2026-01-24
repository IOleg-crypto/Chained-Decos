#include "texture_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"
#include <filesystem>

namespace CHEngine
{
Ref<TextureAsset> TextureAsset::Load(const std::string &path)
{
    std::filesystem::path fullPath = AssetManager::ResolvePath(path);
    if (!std::filesystem::exists(fullPath))
    {
        // Try fallback in Textures/
        std::filesystem::path filename = std::filesystem::path(path).filename();
        fullPath = AssetManager::ResolvePath("Textures/" + filename.string());

        if (!std::filesystem::exists(fullPath))
        {
            CH_CORE_ERROR("Texture file not found: {}", path);
            return nullptr;
        }
    }

    Texture2D tex = ::LoadTexture(fullPath.string().c_str());
    if (tex.id == 0)
        return nullptr;

    auto asset = CreateRef<TextureAsset>();
    asset->m_Texture = tex;
    asset->SetPath(path);
    return asset;
}

TextureAsset::~TextureAsset()
{
    if (m_Texture.id > 0)
        ::UnloadTexture(m_Texture);
    CH_CORE_TRACE("TextureAsset Unloaded: {}", GetPath());
}
} // namespace CHEngine
