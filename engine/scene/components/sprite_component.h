#ifndef CH_SPRITE_COMPONENT_H
#define CH_SPRITE_COMPONENT_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "engine/graphics/texture_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
struct SpriteComponent
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath;
    std::shared_ptr<TextureAsset> Texture;
    Color Tint = WHITE;
    bool FlipX = false;
    bool FlipY = false;
    int ZOrder = 0;

    SpriteComponent() = default;
    SpriteComponent(const SpriteComponent&) = default;
};
} // namespace CHEngine

#endif // CH_SPRITE_COMPONENT_H
