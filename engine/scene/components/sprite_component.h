#ifndef CH_SPRITE_COMPONENT_H
#define CH_SPRITE_COMPONENT_H

#include "engine/core/reflection.h"
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

    SpriteComponent() = default;
    SpriteComponent(const SpriteComponent&) = default;


    CH_REFLECT_BEGIN(SpriteComponent)
        props.Header("Asset");
        props.Handle("TextureHandle", TextureHandle);
        if (props.File("TexturePath", TexturePath, "png,jpg,bmp,tga"))
        {
            TextureHandle = AssetHandle(0);
        }
        
        props.Header("Appearance");
        if (props.BeginGroup("Transform"))
        {
            props.Property("Tint", Tint);
            props.Property("FlipX", FlipX);
            props.Property("FlipY", FlipY);
            props.Property("ZOrder", ZOrder, PropertyMeta(-1000.0f, 1000.0f, 1.0f));
            props.EndGroup();
        }
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_SPRITE_COMPONENT_H
