#include "engine/assets/types/font_asset.h"
#include "engine/graphics/freetype_gl_atlas.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"

namespace Chained
{
	void FontAsset::Unload()
	{
		if (m_Font.freeTypeAtlas)
		{
			delete m_Font.freeTypeAtlas;
			m_Font.freeTypeAtlas = nullptr;
		}
		m_Font = NativeFont();
	}

	NativeFont FontAsset::CreateFromFile(const std::string& path)
	{
		auto* am = ServiceLocator::TryGet<AssetManager>();
		if (!am)
		{
			return NativeFont{};
		}

		auto asset = am->Get<FontAsset>(path);
		if (asset && asset->GetState() == AssetState::Ready)
		{
			return asset->GetFont();
		}
		return NativeFont{};
	}
} // namespace Chained