#include "engine/graphics/freetype_gl_atlas.h"

namespace Chained
{
	FreeTypeGLAtlas::~FreeTypeGLAtlas()
	{
		Shutdown();
	}

	std::string FreeTypeGLAtlas::CodepointToUTF8(uint32_t cp) const
	{
		if (cp < 128)
		{
			return std::string(1, static_cast<char>(cp));
		}

		if (cp < 0x800)
		{
			char bytes[3] = {static_cast<char>(0xC0 | (cp >> 6)), static_cast<char>(0x80 | (cp & 0x3F)), '\0'};
			return std::string(bytes);
		}

		char bytes[4] = {static_cast<char>(0xE0 | (cp >> 12)), static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
						 static_cast<char>(0x80 | (cp & 0x3F)), '\0'};
		return std::string(bytes);
	}

	bool FreeTypeGLAtlas::SetupFont(ftgl::texture_font_t* font)
	{
		if (!font)
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: font creation failed");
			Shutdown();
			return false;
		}

		font->hinting = 1;
		font->kerning = 1;

		PreloadRange(32, 96);
		PreloadRange(0x0400, 256);
		return true;
	}

	bool FreeTypeGLAtlas::Init(const std::string& path, float ptSize, uint32_t atlasW, uint32_t atlasH)
	{
		Shutdown();
		m_AtlasWidth = atlasW;
		m_AtlasHeight = atlasH;
		m_PtSize = ptSize;

		m_Atlas = ftgl::texture_atlas_new(atlasW, atlasH, 1);
		if (!m_Atlas)
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_atlas_new failed ({}x{})", atlasW, atlasH);
			return false;
		}

		m_Font = ftgl::texture_font_new_from_file(m_Atlas, ptSize, path.c_str());
		if (!SetupFont(m_Font))
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_font_new_from_file failed '{}'", path);
			return false;
		}

		CH_CORE_INFO("FreeTypeGLAtlas: loaded '{}' pt={} atlas={}x{}", path, ptSize, atlasW, atlasH);
		return true;
	}

	bool FreeTypeGLAtlas::InitFromMemory(const void* data, size_t dataSize, float ptSize, uint32_t atlasW,
										 uint32_t atlasH)
	{
		Shutdown();
		m_AtlasWidth = atlasW;
		m_AtlasHeight = atlasH;
		m_PtSize = ptSize;

		m_Atlas = ftgl::texture_atlas_new(atlasW, atlasH, 1);
		if (!m_Atlas)
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_atlas_new failed ({}x{})", atlasW, atlasH);
			return false;
		}

		m_Font = ftgl::texture_font_new_from_memory(m_Atlas, ptSize, data, dataSize);
		if (!SetupFont(m_Font))
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_font_new_from_memory failed");
			return false;
		}

		CH_CORE_INFO("FreeTypeGLAtlas: loaded from memory pt={} atlas={}x{}", ptSize, atlasW, atlasH);
		return true;
	}

	void FreeTypeGLAtlas::Shutdown()
	{
		if (m_Font)
		{
			ftgl::texture_font_delete(m_Font);
			m_Font = nullptr;
		}
		if (m_Atlas)
		{
			ftgl::texture_atlas_delete(m_Atlas);
			m_Atlas = nullptr;
		}
		m_FtLibrary = nullptr;
		m_Glyphs.clear();
	}

	const AtlasGlyphMetrics* FreeTypeGLAtlas::GetGlyph(uint32_t codepoint)
	{
		auto it = m_Glyphs.find(codepoint);
		return (it != m_Glyphs.end()) ? &it->second : CacheGlyph(codepoint);
	}

	const AtlasGlyphMetrics* FreeTypeGLAtlas::CacheGlyph(uint32_t codepoint)
	{
		if (!m_Font)
		{
			return nullptr;
		}

		std::string utf8 = CodepointToUTF8(codepoint);
		ftgl::texture_glyph_t* g = ftgl::texture_font_get_glyph(m_Font, utf8.c_str());
		if (!g)
		{
			return nullptr;
		}

		AtlasGlyphMetrics m{codepoint,
							static_cast<uint32_t>(g->width),
							static_cast<uint32_t>(g->height),
							g->offset_x,
							g->offset_y,
							g->advance_x,
							g->s0,
							g->t0,
							g->s1,
							g->t1};

		auto [it, inserted] = m_Glyphs.insert_or_assign(codepoint, m);
		return &it->second;
	}

	void FreeTypeGLAtlas::PreloadRange(uint32_t first, uint32_t count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			CacheGlyph(first + i);
		}
	}

	const uint8_t* FreeTypeGLAtlas::GetAlphaData() const
	{
		return m_Atlas ? m_Atlas->data : nullptr;
	}

} // namespace Chained
