#ifndef CH_FREETYPE_GL_ATLAS_H
#define CH_FREETYPE_GL_ATLAS_H

#include <cstdint>
#include <string>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "texture-atlas.h"
#include "texture-font.h"

namespace Chained
{
	struct AtlasGlyphMetrics
	{
		uint32_t codepoint = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		int offsetX = 0;
		int offsetY = 0;
		float advanceX = 0.0f;
		// UV coordinates into the atlas (normalized 0..1)
		float s0 = 0.0f, t0 = 0.0f;
		float s1 = 0.0f, t1 = 0.0f;
	};

	class FreeTypeGLAtlas
	{
	public:
		FreeTypeGLAtlas() = default;
		~FreeTypeGLAtlas();
		FreeTypeGLAtlas(const FreeTypeGLAtlas&) = delete;
		FreeTypeGLAtlas& operator=(const FreeTypeGLAtlas&) = delete;

		/// Initialise from a .ttf / .otf file on disk.
		/// @param path        Path to the font file (passed to FreeType).
		/// @param ptSize      Font size in points.
		/// @param atlasWidth  Atlas texture width in pixels (power of two recommended).
		/// @param atlasHeight Atlas texture height in pixels.
		/// @return true on success.
		bool Init(const std::string& path, float ptSize, uint32_t atlasWidth = 1024, uint32_t atlasHeight = 1024);

		/// Initialise from font data already loaded in memory.
		bool InitFromMemory(const void* data, size_t dataSize, float ptSize, uint32_t atlasWidth = 1024,
							uint32_t atlasHeight = 1024);

		void Shutdown();

		/// Get or create a glyph.  Returns nullptr on failure.
		const AtlasGlyphMetrics* GetGlyph(uint32_t codepoint);

		/// Ensure every glyph in the given range is loaded (batch prefetch).
		void PreloadRange(uint32_t firstCodepoint, uint32_t count);

		/// Read-only access to the raw alpha atlas (1 byte per pixel).
		const uint8_t* GetAlphaData() const;
		uint32_t GetAtlasWidth() const
		{
			return m_AtlasWidth;
		}
		uint32_t GetAtlasHeight() const
		{
			return m_AtlasHeight;
		}

		/// Raw freetype-gl pointers (for callers that need them directly).
		ftgl::texture_atlas_t* GetAtlas() const
		{
			return m_Atlas;
		}
		ftgl::texture_font_t* GetFont() const
		{
			return m_Font;
		}

		/// Access the cached glyph metrics (populated by GetGlyph / PreloadRange).
		const std::unordered_map<uint32_t, AtlasGlyphMetrics>& GetCachedGlyphs() const
		{
			return m_Glyphs;
		}

	private:
		void CacheGlyph(uint32_t codepoint);

		ftgl::texture_atlas_t* m_Atlas = nullptr;
		ftgl::texture_font_t* m_Font = nullptr;
		FT_Library m_FtLibrary = nullptr;

		uint32_t m_AtlasWidth = 0;
		uint32_t m_AtlasHeight = 0;
		float m_PtSize = 0.0f;

		// codepoint -> metrics cache (owned by us, separate from freetype-gl's vector)
		std::unordered_map<uint32_t, AtlasGlyphMetrics> m_Glyphs;
	};

} // namespace Chained

#endif // CH_FREETYPE_GL_ATLAS_H
