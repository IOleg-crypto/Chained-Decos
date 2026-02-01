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

void TextureAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        SetState(AssetState::Failed);
        return;
    }

    // This runs on background thread (std::async)
    // Raylib's LoadImage only does CPU work and memory allocation
    m_PendingImage = ::LoadImage(path.c_str());
    
    if (m_PendingImage.data != nullptr)
        m_HasPendingImage = true;
    else
        SetState(AssetState::Failed);
}

void TextureAsset::UploadToGPU()
{
    // This MUST run on main thread (where OpenGL context is)
    if (m_HasPendingImage && m_PendingImage.data != nullptr)
    {
        if (m_Texture.id > 0)
            ::UnloadTexture(m_Texture);
            
        m_Texture = ::LoadTextureFromImage(m_PendingImage);
        ::UnloadImage(m_PendingImage);
        
        m_PendingImage.data = nullptr;
        m_HasPendingImage = false;
        
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

} // namespace CHEngine
