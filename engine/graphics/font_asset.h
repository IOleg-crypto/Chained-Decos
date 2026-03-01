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
    static AssetType GetStaticType()
    {
        return AssetType::Font;
    }

    FontAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~FontAsset();
    Font& GetFont()
    {
        return m_Font;
    }
    const Font& GetFont() const
    {
        return m_Font;
    }

    void UploadToGPU()
    {
    } // Required by AssetManager::UpdateCache

private:
    Font m_Font = {0};
};
} // namespace CHEngine

#endif // CH_FONT_ASSET_H
