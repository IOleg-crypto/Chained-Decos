#include "font_asset.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <filesystem>


namespace CHEngine
{
std::shared_ptr<FontAsset> FontAsset::Load(const std::string &path)
{
    auto asset = std::make_shared<FontAsset>();
    asset->SetPath(path);
    asset->LoadFromFile(path);
    return asset->GetState() == AssetState::Ready ? asset : nullptr;
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

    if (path.empty())
    {
        SetState(AssetState::Failed);
        return;
    }

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("Font file not found: {}", path);
        SetState(AssetState::Failed);
        return;
    }

    m_Font = ::LoadFont(fullPath.string().c_str());

    if (m_Font.texture.id > 0)
        SetState(AssetState::Ready);
    else
    {
        CH_CORE_ERROR("Failed to load font: {}", path);
        SetState(AssetState::Failed);
    }
}
} // namespace CHEngine
