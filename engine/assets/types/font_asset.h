#ifndef CH_FONT_ASSET_H
#define CH_FONT_ASSET_H

#include "engine/assets/asset.h"
#include <cstdint>
#include <string>

namespace Chained
{
struct FontChar
{
    float x0, y0, x1, y1;
    float xoff, yoff, xadvance;
};

struct Font
{
    uint32_t textureId = 0;
    int atlasWidth = 0;
    int atlasHeight = 0;
    FontChar chars[128];
    float fontSize = 32.0f;
};

class FontAsset : public Asset
{
public:
    FontAsset()
        : Asset(GetStaticType())
    {
    }
    FontAsset(AssetHandle handle)
        : Asset(GetStaticType(), handle)
    {
    }
    virtual ~FontAsset() override = default;

    static AssetType GetStaticType()
    {
        return AssetType::Font;
    }

    size_t GetMemoryUsage() const override
    {
        return sizeof(*this) + (m_Font.atlasWidth * m_Font.atlasHeight);
    }

    const Font& GetFont() const
    {
        return m_Font;
    }
    void SetFont(const Font& font)
    {
        m_Font = font;
    }

private:
    Font m_Font = {0};
};
} // namespace Chained

#endif // CH_FONT_ASSET_H