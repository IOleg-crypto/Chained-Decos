#ifndef CH_SPRITE_COMPONENT_H
#define CH_SPRITE_COMPONENT_H

#include "engine/assets/asset.h"
#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"


namespace Chained
{
struct SpriteComponent
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath;
    Color Tint = Color::White();
    bool FlipX = false;
    bool FlipY = false;
    int ZOrder = 0;

    static const char* GetStaticName()
    {
        return "SpriteComponent";
    }
};

CH_MARK_RFL(SpriteComponent);
} // namespace Chained

#endif // CH_SPRITE_COMPONENT_H
