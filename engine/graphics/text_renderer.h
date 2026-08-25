#ifndef CH_TEXT_RENDERER_H
#define CH_TEXT_RENDERER_H

#include "engine/assets/types/font_asset.h"
#include "engine/graphics/api/texture.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace Chained
{

	struct GlyphQuad
	{
		float x, y;			  // top-left in pixels
		float w, h;			  // quad size in pixels
		float s0, t0, s1, t1; // UV into atlas
		float advance;		  // horizontal advance for next glyph
	};

	class TextRenderer
	{
	public:
		TextRenderer() = default;
		~TextRenderer()
		{
			Shutdown();
		}

		void Init();
		void Shutdown();

		/// Create / return a GPU texture of the whole text string (legacy path, kept for compat).
		uint32_t GetOrCreateTexture(const std::string& text, const Font& font, float fontSize = 32.0f, int padding = 4);

		int GetLastWidth() const
		{
			return m_LastWidth;
		}
		int GetLastHeight() const
		{
			return m_LastHeight;
		}

		/// Layout glyphs without rasterising a texture.
		/// Returns quads positioned relative to the top-left of the text region.
		std::vector<GlyphQuad> LayoutGlyphs(const std::string& text, const Font& font, float fontSize = 32.0f) const;

		/// Measure text extents without generating quads.
		void Measure(const std::string& text, const Font& font, float fontSize, float& outWidth,
					 float& outHeight) const;

	private:
		struct CachedText
		{
			std::shared_ptr<Texture> gpuTexture;
			int width = 0;
			int height = 0;
		};

		std::unordered_map<std::string, CachedText> m_Cache;
		int m_LastWidth = 0;
		int m_LastHeight = 0;
		bool m_Initialized = false;
	};

} // namespace Chained

#endif // CH_TEXT_RENDERER_H
