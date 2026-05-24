#ifndef CH_SPRITE_COMPONENT_H
#define CH_SPRITE_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/core/reflection_rfl.h"
#include "engine/graphics/texture_system.h"

namespace CHEngine
{
struct SpriteComponent
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath;
    Color Tint = Color::White();
    bool FlipX = false;
    bool FlipY = false;
    int ZOrder = 0;

    static const char* GetStaticName() { return "SpriteComponent"; }
};

CH_MARK_RFL(SpriteComponent);
} // namespace CHEngine

#endif // CH_SPRITE_COMPONENT_H
