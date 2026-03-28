#include "engine/graphics/assets/font_asset.h"

namespace CHEngine
{
FontAsset::~FontAsset()
{
    // For now, we don't have a specific font texture unloader in the native renderer
    // We should handle this through the TextureManager if it was managed there.
    if (m_Font.textureId > 0)
    {
        // Renderer::UnloadTexture(m_Font.textureId);
    }
}
} // namespace CHEngine
