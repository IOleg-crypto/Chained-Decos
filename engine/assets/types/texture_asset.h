#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/texture.h"
#include <memory>
#include <cstdint>

namespace CHEngine
{


class TextureAsset : public Asset, public std::enable_shared_from_this<TextureAsset>
{
public:
    TextureAsset()
        : Asset(GetStaticType())
    {
    }
    TextureAsset(AssetHandle handle)
        : Asset(GetStaticType(), handle)
    {
    }
    virtual ~TextureAsset() = default;


    static AssetType GetStaticType()
    {
        return AssetType::Texture;
    }

    void OnLoaded() override;

    std::shared_ptr<Texture> GetTexture() const { return m_Texture; }
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    
    // Direct GPU Texture API forwards
    uint32_t GetRendererID() const { return m_Texture ? m_Texture->GetRendererID() : 0; }
    void Bind(uint32_t slot = 0) const { if (m_Texture) m_Texture->Bind(slot); }

    bool IsCubemap() const { return m_IsCubemap; }
    bool IsHDR() const { return m_IsHDR; }

    void SetRawData(void* data, int width, int height, int channels, bool isHDR)
    {
        m_RawData = data;
        m_RawWidth = width;
        m_RawHeight = height;
        m_RawChannels = channels;
        m_IsHDR = isHDR;
        m_HasPendingImage = true;
    }
    void SetIsCubemap(bool isCubemap) { m_IsCubemap = isCubemap; }
    void SetIsHDR(bool isHDR) { m_IsHDR = isHDR; }

    void Unload();

public: // Internal data for loader
    std::shared_ptr<Texture> m_Texture;
    void* m_RawData = nullptr;
    int m_RawWidth = 0;
    int m_RawHeight = 0;
    int m_RawChannels = 0;
    bool m_HasPendingImage = false;
    bool m_IsCubemap = false;
    bool m_IsHDR = false;
};

} // namespace CHEngine

#endif // CH_TEXTURE_ASSET_H
