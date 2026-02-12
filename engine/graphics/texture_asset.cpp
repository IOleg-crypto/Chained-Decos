#include "texture_asset.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <filesystem>

namespace CHEngine
{
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
