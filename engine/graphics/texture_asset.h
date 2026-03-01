#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/graphics/asset.h"
#include <memory>
#include <raylib.h>
#include <string>

namespace CHEngine
{

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
    void SetPendingImage(Image image)
    {
        m_PendingImage = image;
        m_HasPendingImage = true;
    }

    Texture2D& GetTexture()
    {
        return m_Texture;
    }
    void SetTexture(Texture2D texture)
    {
        m_Texture = texture;
    }

    bool IsCubemap() const { return m_IsCubemap; }
    void SetIsCubemap(bool isCubemap) { m_IsCubemap = isCubemap; }

    void Unload();

private:
    Texture2D m_Texture = {0};
    Image m_PendingImage = {0};
    bool m_HasPendingImage = false;
    bool m_IsCubemap = false;
};
} // namespace CHEngine

#endif // CH_TEXTURE_ASSET_H
