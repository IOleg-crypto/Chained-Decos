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
        CH_HEADER(props, "Asset");
        CH_HANDLE(props, TextureHandle);
        if (CH_FILE(props, TexturePath, "png,jpg,bmp,tga"))
        {
            TextureHandle = AssetHandle(0);
        }
        
        CH_HEADER(props, "Appearance");
        if (CH_BEGIN_GROUP(props, "Transform", true))
        {
            CH_PROP(props, Tint);
            CH_PROP(props, FlipX);
            CH_PROP(props, FlipY);
            CH_PROP_META(props, ZOrder, PropertyMeta(-1000.0f, 1000.0f, 1.0f));
            CH_END_GROUP(props);
        }
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_SPRITE_COMPONENT_H
