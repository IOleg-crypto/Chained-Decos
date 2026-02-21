#include "font_asset.h"

namespace CHEngine
{
FontAsset::~FontAsset()
{
    if (m_Font.texture.id > 0)
    {
        UnloadFont(m_Font);
    }
}
} // namespace CHEngine
