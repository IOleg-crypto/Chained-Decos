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
    // Check state first to avoid redundant work/races
    AssetState currentState = GetState();
    if (currentState == AssetState::Ready || currentState == AssetState::Failed)
    {
        return;
    }

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
            CH_CORE_WARN("TextureAsset: Attempting diagnostic HDR load for '{}'", m_Path);
            if (m_Texture.id > 0) ::UnloadTexture(m_Texture);
            
            // Try loading image first to see where it fails
            Image img = ::LoadImage(m_Path.c_str());
            if (img.data != nullptr)
            {
                CH_CORE_INFO("TextureAsset: HDR Image loaded to CPU ({}x{}, format: {})", img.width, img.height, img.format);
                
                // Flip vertically because stb_image loads top-down, but cubemap generation expects GL-style bottom-up or standard spherical UVs
                ::ImageFlipVertical(&img);
                
                m_Texture = ::LoadTextureFromImage(img);
                ::UnloadImage(img);
            }
            else
            {
                 CH_CORE_ERROR("TextureAsset: Failed to load HDR Image from path: {}", m_Path);
                 // Check if file exists to distinguish between "missing file" and "unsupported format"
                 if (!std::filesystem::exists(m_Path)) {
                     CH_CORE_ERROR("TextureAsset: File DOES NOT EXIST at path: {}", m_Path);
                 } else {
                     CH_CORE_ERROR("TextureAsset: File exists but raylib failed to parse it (likely missing HDR support in build)");
                 }
            }

            if (m_Texture.id > 0)
            {
                ::SetTextureFilter(m_Texture, TEXTURE_FILTER_BILINEAR);
                SetState(AssetState::Ready);
                CH_CORE_INFO("TextureAsset: HDR texture created on GPU (ID: {}, format: {})", 
                             m_Texture.id, m_Texture.format);
            }
            else
            {
                CH_CORE_ERROR("TextureAsset: Failed to create GPU texture for HDR '{}'", m_Path);
                SetState(AssetState::Failed);
            }
        }
    }
    else
    {
        // State is None or Failed
        CH_CORE_WARN("TextureAsset::UploadToGPU: Called with state {}, doing nothing for '{}'", (int)m_State.load(), m_Path);
    }
}

TextureAsset::~TextureAsset()
{
    Unload();
}

void TextureAsset::Unload()
{
    if (m_Texture.id > 0)
    {
        ::UnloadTexture(m_Texture);
        m_Texture.id = 0;
    }
    if (m_PendingImage.data != nullptr)
    {
        ::UnloadImage(m_PendingImage);
        m_PendingImage.data = nullptr;
    }
}

} // namespace CHEngine
