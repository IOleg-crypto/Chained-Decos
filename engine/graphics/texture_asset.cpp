#include "texture_asset.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<TextureAsset> TextureAsset::Load(const std::string &path)
{
    if (path.empty())
        return nullptr;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        // Try fallback search in asset directory if path is relative
        auto filename = fullPath.filename();
        // For baseline build, we just use path as is
    }

    if (!std::filesystem::exists(fullPath))
        return nullptr;

    Image image = ::LoadImage(fullPath.string().c_str());
    if (image.data == nullptr)
        return nullptr;

    Texture2D texture = ::LoadTextureFromImage(image);
    ::UnloadImage(image);

    auto asset = std::make_shared<TextureAsset>();
    asset->SetTexture(texture);
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);

    return asset;
}

void TextureAsset::LoadAsync(const std::string &path)
{
    // Simplified background loading for baseline build
    auto loaded = Load(path);
}

void TextureAsset::UploadToGPU()
{
    if (m_PendingImage.data != nullptr)
    {
        if (m_Texture.id > 0)
            UnloadTexture(m_Texture);
        m_Texture = LoadTextureFromImage(m_PendingImage);
        UnloadImage(m_PendingImage);
        m_PendingImage.data = nullptr;
        SetState(AssetState::Ready);
    }
}

TextureAsset::~TextureAsset()
{
    if (m_Texture.id > 0)
    {
        ::UnloadTexture(m_Texture);
    }
}

void TextureAsset::LoadFromFile(const std::string &path)
{
    // This was used for async loading, simplified for now
}

} // namespace CHEngine
