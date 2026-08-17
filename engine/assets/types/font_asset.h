#ifndef CH_FONT_ASSET_H
#define CH_FONT_ASSET_H

#include "engine/assets/asset.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include "engine/graphics/api/texture.h"

namespace Chained
{
	class FreeTypeGLAtlas;

	struct NativeFontChar
	{
		float x0, y0, x1, y1; // Texture coordinates
		float xoff, yoff, xadvance;
	};

	struct NativeFont
	{
		std::shared_ptr<Texture> textureAtlas = nullptr;
		int atlasWidth = 0;
		int atlasHeight = 0;
		std::unordered_map<uint32_t, NativeFontChar> chars;
		float fontSize = 32.0f;
		std::vector<uint8_t> atlasPixels; // CPU-side RGBA8 copy for text rasterization

		// Direct access to the freetype-gl atlas (owned by FontAsset)
		FreeTypeGLAtlas* freeTypeAtlas = nullptr;

		const NativeFontChar& GetChar(uint32_t codepoint) const
		{
			static const NativeFontChar empty = {0, 0, 0, 0, 0, 0, 0};
			auto it = chars.find(codepoint);
			return (it != chars.end()) ? it->second : empty;
		}
	};

	class FontAsset : public Asset
	{
	public:
		FontAsset()
			: Asset(GetStaticType())
		{
		}
		virtual ~FontAsset() = default;

		static AssetType GetStaticType()
		{
			return AssetType::Font;
		}

		void OnLoaded() override
		{
		}

		void Unload() override;

		const NativeFont& GetFont() const
		{
			return m_Font;
		}
		void SetFont(const NativeFont& font)
		{
			m_Font = font;
		}

		// Hazel-style asset loading
		static NativeFont CreateFromFile(const std::string& path);

	private:
		NativeFont m_Font = {0};
	};
} // namespace Chained

#endif // CH_FONT_ASSET_H
