#ifndef CH_SPRITE_COMPONENT_H
#define CH_SPRITE_COMPONENT_H

#include "engine/core/base.h"
#include "engine/core/assets/asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include <memory>
#include <string>

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

    static const char* GetStaticName() { return "SpriteComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, SpriteComponent& component)
    {
        archive.Handle("TextureHandle", component.TextureHandle)
            .Path("TexturePath", component.TexturePath)
            .Property("Tint", component.Tint)
            .Property("FlipX", component.FlipX)
            .Property("FlipY", component.FlipY)
            .Property("ZOrder", component.ZOrder);
    }
};
} // namespace CHEngine

#endif // CH_SPRITE_COMPONENT_H
