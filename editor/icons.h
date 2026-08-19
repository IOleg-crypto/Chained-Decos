
#ifndef CH_EDITOR_ICONS_H
#define CH_EDITOR_ICONS_H

#include <memory>

namespace Chained
{
	class TextureAsset;

	// Cached editor icon textures used during scene rendering.
	struct EditorIcons
	{
		std::shared_ptr<TextureAsset> LightIcon;
		std::shared_ptr<TextureAsset> SpawnIcon;
		std::shared_ptr<TextureAsset> CameraIcon;
		std::shared_ptr<TextureAsset> AudioIcon;
	};
} // namespace Chained

#endif // CH_EDITOR_ICONS_H
