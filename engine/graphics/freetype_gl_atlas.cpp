#include "engine/graphics/freetype_gl_atlas.h"
#include "engine/core/log.h"

namespace Chained
{
	FreeTypeGLAtlas::~FreeTypeGLAtlas()
	{
		Shutdown();
	}

	// ── helpers ──────────────────────────────────────────────────────────────
	static std::string CodepointToUTF8(uint32_t cp)
	{
		std::string s;
		if (cp < 0x80)
		{
			s += static_cast<char>(cp);
		}
		else if (cp < 0x800)
		{
			s += static_cast<char>(0xC0 | (cp >> 6));
			s += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else if (cp < 0x10000)
		{
			s += static_cast<char>(0xE0 | (cp >> 12));
			s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			s += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else
		{
			s += static_cast<char>(0xF0 | (cp >> 18));
			s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			s += static_cast<char>(0x80 | (cp & 0x3F));
		}
		return s;
	}

	// ── init ─────────────────────────────────────────────────────────────────
	bool FreeTypeGLAtlas::Init(const std::string& path, float ptSize, uint32_t atlasW, uint32_t atlasH)
	{
		Shutdown();
		m_AtlasWidth = atlasW;
		m_AtlasHeight = atlasH;
		m_PtSize = ptSize;

		m_Atlas = ftgl::texture_atlas_new(atlasW, atlasH, 1); // depth 1 = alpha only
		if (!m_Atlas)
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_atlas_new failed ({}x{})", atlasW, atlasH);
			return false;
		}

		m_Font = ftgl::texture_font_new_from_file(m_Atlas, ptSize, path.c_str());
		if (!m_Font)
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_font_new_from_file failed '{}'", path);
			return false;
		}
		m_Font->hinting = 1;
		m_Font->kerning = 1;

		// Preload ASCII (32–127) and Cyrillic (0x0400–0x04FF)
		PreloadRange(32, 96);
		PreloadRange(0x0400, 256);

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
		if (!m_Font)
		{
			CH_CORE_ERROR("FreeTypeGLAtlas: texture_font_new_from_memory failed");
			return false;
		}
		m_Font->hinting = 1;
		m_Font->kerning = 1;

		PreloadRange(32, 96);
		PreloadRange(0x0400, 256);

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

	// ── glyph access ─────────────────────────────────────────────────────────
	const AtlasGlyphMetrics* FreeTypeGLAtlas::GetGlyph(uint32_t codepoint)
	{
		if (!m_Font)
		{
			return nullptr;
		}
		auto it = m_Glyphs.find(codepoint);
		if (it != m_Glyphs.end())
		{
			return &it->second;
		}
		CacheGlyph(codepoint);
		auto it2 = m_Glyphs.find(codepoint);
		return (it2 != m_Glyphs.end()) ? &it2->second : nullptr;
	}

	void FreeTypeGLAtlas::CacheGlyph(uint32_t codepoint)
	{
		std::string utf8 = CodepointToUTF8(codepoint);
		// texture_font_get_glyph renders + packs if not already present
		ftgl::texture_glyph_t* g = ftgl::texture_font_get_glyph(m_Font, utf8.c_str());
		if (!g)
		{
			return;
		}
		AtlasGlyphMetrics m;
		m.codepoint = codepoint;
		m.width = static_cast<uint32_t>(g->width);
		m.height = static_cast<uint32_t>(g->height);
		m.offsetX = g->offset_x;
		m.offsetY = g->offset_y;
		m.advanceX = g->advance_x;
		m.s0 = g->s0;
		m.t0 = g->t0;
		m.s1 = g->s1;
		m.t1 = g->t1;
		m_Glyphs[codepoint] = m;
	}

	void FreeTypeGLAtlas::PreloadRange(uint32_t first, uint32_t count)
	{
		if (!m_Font)
		{
			return;
		}
		for (uint32_t i = 0; i < count; ++i)
		{
			uint32_t cp = first + i;
			std::string utf8 = CodepointToUTF8(cp);
			ftgl::texture_glyph_t* g = ftgl::texture_font_get_glyph(m_Font, utf8.c_str());
			if (!g)
			{
				continue;
			}
			AtlasGlyphMetrics m;
			m.codepoint = cp;
			m.width = static_cast<uint32_t>(g->width);
			m.height = static_cast<uint32_t>(g->height);
			m.offsetX = g->offset_x;
			m.offsetY = g->offset_y;
			m.advanceX = g->advance_x;
			m.s0 = g->s0;
			m.t0 = g->t0;
			m.s1 = g->s1;
			m.t1 = g->t1;
			m_Glyphs[cp] = m;
		}
	}

	const uint8_t* FreeTypeGLAtlas::GetAlphaData() const
	{
		return m_Atlas ? m_Atlas->data : nullptr;
	}

} // namespace Chained
