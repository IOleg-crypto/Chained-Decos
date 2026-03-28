#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/core/assets/asset.h"
#include <memory>
#include <string>
#include <cstdint>

namespace CHEngine
{

struct Texture
{
    uint32_t id = 0;
    int width = 0;
    int height = 0;
    int mipmaps = 0;
    int format = 0;
};

struct RawImage
{
    void* data = nullptr;
    int width = 0;
    int height = 0;
    int mipmaps = 0;
    int format = 0;
    int channels = 0;
    bool isHDR = false;
};

class TextureAsset : public Asset
{
public:
    static AssetType GetStaticType()
    {
        return AssetType::Texture;
    }

    TextureAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~TextureAsset();

    void UploadToGPU();

    // For internal use by Importer
    void SetPendingImage(const RawImage& image)
    {
        m_PendingImage = image;
        m_HasPendingImage = true;
    }

    Texture& GetTexture()
    {
        return m_Texture;
    }
    void SetTexture(const Texture& texture)
    {
        m_Texture = texture;
    }

    bool IsCubemap() const { return m_IsCubemap; }
    void SetIsCubemap(bool isCubemap) { m_IsCubemap = isCubemap; }

    bool IsHDR() const {
        return m_Texture.format >= 8 && m_Texture.format <= 13;
    }

    void Unload();

private:
    Texture m_Texture = {0};
    RawImage m_PendingImage = {0};
    bool m_HasPendingImage = false;
    bool m_IsCubemap = false;
};
} // namespace CHEngine

#endif // CH_TEXTURE_ASSET_H
