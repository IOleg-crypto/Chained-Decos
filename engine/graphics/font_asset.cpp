#include "font_asset.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <filesystem>


namespace CHEngine
{
std::shared_ptr<FontAsset> FontAsset::Load(const std::string &path)
{
    if (path.empty())
        return nullptr;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("Font file not found: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<FontAsset>();
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);
    asset->m_Font = LoadFont(fullPath.string().c_str());

    if (asset->m_Font.texture.id == 0)
    {
        CH_CORE_ERROR("Failed to load font: {}", path);
        return nullptr;
    }

    return asset;
}

FontAsset::~FontAsset()
{
    if (m_Font.texture.id > 0)
    {
        UnloadFont(m_Font);
    }
}
void FontAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    auto loaded = Load(path);
    if (loaded)
    {
        m_Font = loaded->m_Font;
        loaded->m_Font.texture.id = 0; // Prevent double unload
        SetState(AssetState::Ready);
    }
    else
    {
        SetState(AssetState::Failed);
    }
}
} // namespace CHEngine
