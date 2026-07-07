#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/texture.h"
#include <memory>
// #include <string>
#include <cstdint>

namespace Chained
{

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
    TextureAsset()
        : Asset(GetStaticType())
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

    bool IsCubemap() const { return m_IsCubemap; }
    bool IsHDR() const { return m_IsHDR; }

    void SetPendingImage(const RawImage& image) { m_PendingImage = image; m_HasPendingImage = true; }
    void SetIsCubemap(bool isCubemap) { m_IsCubemap = isCubemap; }
    void SetIsHDR(bool isHDR) { m_IsHDR = isHDR; }

    void Unload();

private:
    std::shared_ptr<Texture> m_Texture;
    RawImage m_PendingImage = {0};
    bool m_HasPendingImage = false;
    bool m_IsCubemap = false;
    bool m_IsHDR = false;
};

} // namespace Chained

#endif // CH_TEXTURE_ASSET_H