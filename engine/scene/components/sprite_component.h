#ifndef CH_SPRITE_COMPONENT_H
#define CH_SPRITE_COMPONENT_H

#include "engine/core/reflection.h"

namespace CHEngine
{
struct SpriteComponent
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath;
    std::shared_ptr<TextureAsset> Texture;
    Color Tint = Color::White();
    bool FlipX = false;
    bool FlipY = false;
    int ZOrder = 0;

    SpriteComponent() = default;
    SpriteComponent(const SpriteComponent&) = default;

    CH_REFLECT_BEGIN(SpriteComponent)
        props.Handle("Texture Handle", TextureHandle);
        props.File("Texture Path", TexturePath, "png,jpg,bmp,tga");
        props.Property("Tint", Tint);
        props.Property("Flip X", FlipX);
        props.Property("Flip Y", FlipY);
        props.Property("Z Order", ZOrder);
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_SPRITE_COMPONENT_H
