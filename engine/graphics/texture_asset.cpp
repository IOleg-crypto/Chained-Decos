#include "texture_asset.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "raylib.h"
#include <filesystem>

namespace CHEngine
{

// Maps TextureFilter enum to raylib TextureFilter constant
static int ToRaylibFilter(TextureFilter filter)
{
    switch (filter)
    {
    case TextureFilter::None:          return TEXTURE_FILTER_POINT;
    case TextureFilter::Bilinear:      return TEXTURE_FILTER_BILINEAR;
    case TextureFilter::Trilinear:     return TEXTURE_FILTER_TRILINEAR;
    case TextureFilter::Anisotropic4x: return TEXTURE_FILTER_ANISOTROPIC_4X;
    case TextureFilter::Anisotropic8x: return TEXTURE_FILTER_ANISOTROPIC_8X;
    case TextureFilter::Anisotropic16x:return TEXTURE_FILTER_ANISOTROPIC_16X;
    default:                           return TEXTURE_FILTER_BILINEAR;
    }
}

void TextureAsset::UploadToGPU()
{
    // This MUST run on main thread (where OpenGL context is)
    if (m_HasPendingImage && m_PendingImage.data != nullptr)
    {
        if (m_Texture.id > 0)
        {
            ::UnloadTexture(m_Texture);
        }

        m_Texture = ::LoadTextureFromImage(m_PendingImage);
        ::UnloadImage(m_PendingImage);

        m_PendingImage.data = nullptr;
        m_HasPendingImage = false;

        if (m_Texture.id == 0)
        {
            CH_CORE_ERROR("TextureAsset::UploadToGPU: Failed to create GPU texture!");
            SetState(AssetState::Failed);
            return;
        }

        // Apply texture quality from project config (safe fallback when no project)
        TextureSettings texSettings;
        if (auto project = Project::GetActive())
        {
            texSettings = project->GetConfig().Texture;
        }

        if (texSettings.GenerateMipmaps)
        {
            ::GenTextureMipmaps(&m_Texture);
        }

        ::SetTextureFilter(m_Texture, ToRaylibFilter(texSettings.Filter));

        SetState(AssetState::Ready);
    }
    else if (m_State == AssetState::Loading)
    {
        // Special case for HDR: load directly from path on main thread if no pending image
        std::string ext = std::filesystem::path(m_Path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".hdr")
        {
            CH_CORE_INFO("TextureAsset: Loading HDR texture directly via LoadTexture: '{}'", m_Path);
            if (m_Texture.id > 0) ::UnloadTexture(m_Texture);
            m_Texture = ::LoadTexture(m_Path.c_str());
            
            if (m_Texture.id > 0)
            {
                ::SetTextureFilter(m_Texture, TEXTURE_FILTER_BILINEAR);
                SetState(AssetState::Ready);
                CH_CORE_INFO("TextureAsset: HDR texture loaded successfully (ID: {}, format: {})", 
                             m_Texture.id, m_Texture.format);
            }
            else
            {
                CH_CORE_ERROR("TextureAsset: Failed to load HDR texture directly for '{}'", m_Path);
                SetState(AssetState::Failed);
            }
        }
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
