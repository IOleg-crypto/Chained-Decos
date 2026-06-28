#include "engine/assets/types/texture_asset.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/project/project.h"
#include <filesystem>
#include <algorithm>
#include <stb_image.h>

namespace Chained
{

uint32_t TextureAsset::GetWidth() const
{
    if (m_Texture) return m_Texture->GetWidth();
    return m_PendingImage.width;
}

uint32_t TextureAsset::GetHeight() const
{
    if (m_Texture) return m_Texture->GetHeight();
    return m_PendingImage.height;
}

void TextureAsset::OnLoaded()
{
    CH_PROFILE_FUNCTION();

    if (GetState() == AssetState::Ready || GetState() == AssetState::Failed)
    {
        return;
    }

        if (m_HasPendingImage && m_PendingImage.data != nullptr)
    {
        TextureFormat format = TextureFormat::RGBA8;
        if (m_IsHDR)
        {
            format = (m_PendingImage.channels == 3) ? TextureFormat::RGB16F : TextureFormat::RGBA16F;
        }
        else
        {
            format = (m_PendingImage.channels == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
        }

        if (m_IsCubemap)
        {
            m_Texture = Texture::CreateCubemap(m_PendingImage.width, format);
        }
        else
        {
            m_Texture = Texture::Create(m_PendingImage.width, m_PendingImage.height, format);
            if (m_Texture)
            {
                m_Texture->SetData(m_PendingImage.data, 0); 
            }
        }

        if (m_PendingImage.data != nullptr)
        {
            stbi_image_free(m_PendingImage.data);
            m_PendingImage.data = nullptr;
        }
        m_HasPendingImage = false;

        SetState(AssetState::Ready);
    }
}

void TextureAsset::Unload()
{
    m_Texture.reset();
    if (m_PendingImage.data != nullptr)
    {
        stbi_image_free(m_PendingImage.data);
        m_PendingImage.data = nullptr;
    }
}

} // namespace Chained