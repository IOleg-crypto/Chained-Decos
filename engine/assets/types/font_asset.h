#ifndef CH_FONT_ASSET_H
#define CH_FONT_ASSET_H

#include "engine/assets/asset.h"
#include <cstdint>
#include <memory>
#include "engine/graphics/api/texture.h"

namespace Chained
{
struct NativeFontChar
{
    float x0, y0, x1, y1; // Texture coordinates
    float xoff, yoff, xadvance;
};

struct NativeFont
{
    std::shared_ptr<Texture> textureAtlas = nullptr;
    int atlasWidth = 0;
    int atlasHeight = 0;
    NativeFontChar chars[128]; // ASCII for now
    float fontSize = 32.0f;
};

class FontAsset : public Asset
{
public:
    FontAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~FontAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Font;
    }

    void OnLoaded() override
    {
    }

    const NativeFont& GetFont() const
    {
        return m_Font;
    }
    void SetFont(const NativeFont& font)
    {
        m_Font = font;
    }

    // Hazel-style asset loading
    static NativeFont CreateFromFile(const std::string& path);

private:
    NativeFont m_Font = {0};
};
} // namespace Chained

#endif // CH_FONT_ASSET_H