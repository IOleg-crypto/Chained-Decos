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

    bool IsCubemap() const
    {
        return m_IsCubemap;
    }
    void SetIsCubemap(bool isCubemap)
    {
        m_IsCubemap = isCubemap;
    }

private:
    bool m_IsCubemap = false;
};
} // namespace CHEngine

#endif // CH_TEXTURE_ASSET_H
