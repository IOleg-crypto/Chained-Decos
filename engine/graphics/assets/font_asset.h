#ifndef CH_FONT_ASSET_H
#define CH_FONT_ASSET_H

#include "engine/core/assets/asset.h"
// #include <string>
#include <cstdint>
#include <memory>

namespace CHEngine
{
struct NativeFontChar
{
    float x0, y0, x1, y1; // Texture coordinates
    float xoff, yoff, xadvance;
};

struct NativeFont
{
    uint32_t textureId = 0;
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

    void OnLoaded() override {}

    const NativeFont& GetFont() const { return m_Font; }
    void SetFont(const NativeFont& font) { m_Font = font; }

    // Hazel-style asset loading
    static NativeFont CreateFromFile(const std::string& path);

private:
    NativeFont m_Font = {0};
};
} // namespace CHEngine

#endif // CH_FONT_ASSET_H
