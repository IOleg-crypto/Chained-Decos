#include "texture_asset.h"
#include "engine/core/log.h"
#include "engine/graphics/api/texture.h"
#include <stb_image.h>

namespace Chained
{

TextureAsset::~TextureAsset()
{
    if (m_RawData)
    {
        stbi_image_free(m_RawData);
        m_RawData = nullptr;
    }
}

bool TextureAsset::Finalize()
{
    if (!m_HasPendingImage || !m_RawData)
        return m_Texture != nullptr;

    TextureFormat format;
    if (m_IsHDR)
        format = (m_RawChannels == 3) ? TextureFormat::RGB16F : TextureFormat::RGBA16F;
    else
        format = (m_RawChannels == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;

    if (m_IsCubemap)
        m_Texture = Texture::CreateCubemap(m_RawWidth, format);
    else
        m_Texture = Texture::Create(m_RawWidth, m_RawHeight, format);

    if (m_Texture)
        m_Texture->SetData(m_RawData, 0);
    else
        CH_CORE_ERROR("TextureAsset::Finalize: Texture::Create failed ({} x {})", m_RawWidth, m_RawHeight);

    stbi_image_free(m_RawData);
    m_RawData         = nullptr;
    m_HasPendingImage = false;

    SetState(m_Texture ? AssetState::Ready : AssetState::Failed);
    return m_Texture != nullptr;
}

} // namespace Chained