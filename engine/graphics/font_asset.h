#ifndef CH_FONT_ASSET_H
#define CH_FONT_ASSET_H

#include "asset.h"
#include "raylib.h"
#include <string>

namespace CHEngine
{
class FontAsset : public Asset
{
public:
    FontAsset() = default;
    virtual ~FontAsset();

    static std::shared_ptr<FontAsset> Load(const std::string &path);
    void LoadFromFile(const std::string &path);
    Font &GetFont()
    {
        return m_Font;
    }
    const Font &GetFont() const
    {
        return m_Font;
    }

    virtual AssetType GetType() const override
    {
        return AssetType::Font;
    }

private:
    Font m_Font = { 0 };
};
} // namespace CHEngine

#endif // CH_FONT_ASSET_H
