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
		uint64_t TextureUUID = 0;
		Color Tint = Color::White();
		bool FlipX = false;
		bool FlipY = false;
		int ZOrder = 0;

		static const char* GetStaticName()
		{
			return "SpriteComponent";
		}

		struct UI
		{
			UIMeta TextureHandle = {.ReadOnly = true, .Transient = true};
			UIMeta TexturePath = {.Hint = PropertyMeta::WidgetHint::FilePicker,
								  .Extensions = ".png,.jpg,.jpeg,.tga,.bmp",
								  .Tooltip = "Path to the sprite texture image"};
			UIMeta TextureUUID = {.ReadOnly = true};
			UIMeta Tint = {.Hint = PropertyMeta::WidgetHint::ColorPicker,
						   .Tooltip = "Color multiplier applied to the sprite"};
			UIMeta ZOrder = {.Speed = 1.0f};
		};
	};

	CH_MARK_RFL(SpriteComponent);
} // namespace Chained

#endif // CH_SPRITE_COMPONENT_H
