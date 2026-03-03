#ifndef CH_FONT_ASSET_H
#define CH_FONT_ASSET_H

#include "engine/graphics/asset.h"

namespace CHEngine
{
class FontAsset : public Asset
{
public:
    static AssetType GetStaticType()
    {
        return AssetType::Font;
    }

    FontAsset()
        : Asset(GetStaticType())
    {
    }
};
} // namespace CHEngine

#endif // CH_FONT_ASSET_H
