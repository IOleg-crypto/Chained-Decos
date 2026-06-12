#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/texture.h"
#include <cstdint>
#include <memory>

namespace Chained
{
class TextureAsset : public Asset
{
public:
    TextureAsset(std::shared_ptr<Texture> texture = nullptr)
        : Asset(GetStaticType()),
          m_Texture(texture)
    {
        if (m_Texture)
        {
            m_IsCubemap = m_Texture->GetType() == TextureType::Cubemap;
            SetState(AssetState::Ready);
        }
    }

    TextureAsset(AssetHandle handle, std::shared_ptr<Texture> texture = nullptr)
        : Asset(GetStaticType(), handle),
          m_Texture(texture)
    {
        if (m_Texture)
        {
            m_IsCubemap = m_Texture->GetType() == TextureType::Cubemap;
            SetState(AssetState::Ready);
        }
    }

    virtual ~TextureAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Texture;
    }

    std::shared_ptr<Texture> GetTexture() const
    {
        return m_Texture;
    }
    uint32_t GetWidth() const
    {
        return m_Texture ? m_Texture->GetWidth() : 0;
    }
    uint32_t GetHeight() const
    {
        return m_Texture ? m_Texture->GetHeight() : 0;
    }

    uint32_t GetRendererID() const
    {
        return m_Texture ? m_Texture->GetRendererID() : 0;
    }
    void Bind(uint32_t slot = 0) const
    {
        if (m_Texture)
        {
            m_Texture->Bind(slot);
        }
    }

    bool IsCubemap() const
    {
        return m_IsCubemap;
    }
    bool IsHDR() const
    {
        return m_IsHDR;
    }

    void SetIsCubemap(bool isCubemap)
    {
        m_IsCubemap = isCubemap;
    }
    void SetIsHDR(bool isHDR)
    {
        m_IsHDR = isHDR;
    }

    size_t GetMemoryUsage() const override
    {
        return m_Texture ? (m_Texture->GetWidth() * m_Texture->GetHeight() * 4) : 0;
    }

private:
    std::shared_ptr<Texture> m_Texture;
    bool m_IsCubemap = false;
    bool m_IsHDR = false;
};
} // namespace Chained

#endif // CH_TEXTURE_ASSET_H