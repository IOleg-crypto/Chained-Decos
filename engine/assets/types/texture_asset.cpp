#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/loaders/texture_loader.h"
#include <stb_image.h>

namespace CHEngine
{
    uint32_t TextureAsset::GetWidth() const
    {
        if (m_Texture) return m_Texture->GetWidth();
        return m_RawWidth;
    }

    uint32_t TextureAsset::GetHeight() const
    {
        if (m_Texture) return m_Texture->GetHeight();
        return m_RawHeight;
    }

    void TextureAsset::OnLoaded()
    {
        TextureLoader::Finalize(std::static_pointer_cast<TextureAsset>(shared_from_this()));
    }

    void TextureAsset::Unload()
    {
        m_Texture.reset();
        if (m_RawData != nullptr)
        {
            stbi_image_free(m_RawData);
            m_RawData = nullptr;
        }
    }
} // namespace CHEngine
