#include "texture_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<TextureAsset> TextureAsset::Load(const std::string &path)
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

    auto asset = std::make_shared<TextureAsset>();
    asset->m_Texture = tex;
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);
    return asset;
}

void TextureAsset::LoadAsync(const std::string &path)
{
    std::filesystem::path fullPath = AssetManager::ResolvePath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("Texture file not found (Async): {}", path);
        return;
    }

    // Heavy CPU work: Parsing file into Image data
    Image image = ::LoadImage(fullPath.string().c_str());
    if (image.data == nullptr)
    {
        CH_CORE_ERROR("Failed to load image data (Async): {}", path);
        return;
    }

    // Find the placeholder in the cache
    auto asset = AssetManager::Get<TextureAsset>(path);
    if (asset)
    {
        asset->m_PendingImage = image;
        asset->m_HasPendingImage = true;
        // Queue for GPU upload on main thread
        AssetManager::QueueForGPUUpload(asset);
    }
}

void TextureAsset::UploadToGPU()
{
    if (m_HasPendingImage)
    {
        m_Texture = ::LoadTextureFromImage(m_PendingImage);
        ::UnloadImage(m_PendingImage);
        m_HasPendingImage = false;
        SetState(AssetState::Ready);
        CH_CORE_TRACE("TextureAsset Sync-ed to GPU: {}", GetPath());
    }
}

TextureAsset::~TextureAsset()
{
    if (m_Texture.id > 0)
        ::UnloadTexture(m_Texture);
    CH_CORE_TRACE("TextureAsset Unloaded: {}", GetPath());
}
} // namespace CHEngine
