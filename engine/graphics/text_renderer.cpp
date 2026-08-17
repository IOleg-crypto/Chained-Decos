#include "engine/graphics/text_renderer.h"
#include "engine/graphics/api/texture.h"
#include "engine/graphics/freetype_gl_atlas.h"
#include "engine/assets/types/font_asset.h"
#include <vector>
#include <algorithm>
#include <cstring>

namespace Chained
{

	static std::vector<uint32_t> DecodeUTF8(const std::string& text)
	{
		std::vector<uint32_t> codepoints;
		codepoints.reserve(text.size());
		size_t i = 0;
		const size_t len = text.size();
		while (i < len)
		{
			uint32_t cp = 0;
			unsigned char c = static_cast<unsigned char>(text[i]);
			if (c < 0x80)
			{
				cp = c;
				i += 1;
			}
			else if ((c & 0xE0) == 0xC0 && (i + 1 < len))
			{
				cp = (c & 0x1F) << 6;
				cp |= (static_cast<unsigned char>(text[i + 1]) & 0x3F);
				i += 2;
			}
			else if ((c & 0xF0) == 0xE0 && (i + 2 < len))
			{
				cp = (c & 0x0F) << 12;
				cp |= (static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6;
				cp |= (static_cast<unsigned char>(text[i + 2]) & 0x3F);
				i += 3;
			}
			else if ((c & 0xF8) == 0xF0 && (i + 3 < len))
			{
				cp = (c & 0x07) << 18;
				cp |= (static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12;
				cp |= (static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6;
				cp |= (static_cast<unsigned char>(text[i + 3]) & 0x3F);
				i += 4;
			}
			else
			{
				cp = '?';
				i += 1;
			}
			codepoints.push_back(cp);
		}
		return codepoints;
	}

	void TextRenderer::Init()
	{
		m_Initialized = true;
	}

	void TextRenderer::Shutdown()
	{
		m_Cache.clear();
		m_Initialized = false;
	}

	// ── glyph layout ────────────────────────────────────────────────────────

	void TextRenderer::Measure(const std::string& text, const NativeFont& font, float fontSize, float& outWidth,
							   float& outHeight) const
	{
		if (text.empty())
		{
			outWidth = 0;
			outHeight = 0;
			return;
		}
		float scale = (font.fontSize > 0.0f) ? (fontSize / font.fontSize) : 1.0f;
		std::vector<uint32_t> codepoints = DecodeUTF8(text);

		float cursorX = 0.0f;
		float maxH = 0.0f;
		for (uint32_t cp : codepoints)
		{
			const auto& ch = font.GetChar(cp);
			if (ch.xadvance > 0.0f)
			{
				cursorX += ch.xadvance * scale;
			}
			else
			{
				cursorX += fontSize * 0.5f;
			}
			float glyphH = (ch.y1 - ch.y0) * font.atlasHeight * scale;
			if (glyphH > maxH)
			{
				maxH = glyphH;
			}
		}
		if (maxH <= 0.0f)
		{
			maxH = fontSize;
		}
		outWidth = cursorX;
		outHeight = maxH;
	}

	std::vector<GlyphQuad> TextRenderer::LayoutGlyphs(const std::string& text, const NativeFont& font,
													  float fontSize) const
	{
		std::vector<GlyphQuad> quads;
		if (text.empty())
		{
			return quads;
		}

		float scale = (font.fontSize > 0.0f) ? (fontSize / font.fontSize) : 1.0f;
		std::vector<uint32_t> codepoints = DecodeUTF8(text);
		quads.reserve(codepoints.size());

		float cursorX = 0.0f;

		for (uint32_t cp : codepoints)
		{
			const auto& ch = font.GetChar(cp);

			float gw = (ch.x1 - ch.x0) * font.atlasWidth * scale;
			float gh = (ch.y1 - ch.y0) * font.atlasHeight * scale;

			if (gw > 0.0f && gh > 0.0f)
			{
				GlyphQuad q;
				q.x = cursorX + ch.xoff * scale;
				q.y = fontSize + ch.yoff * scale; // y-off from baseline
				q.w = gw;
				q.h = gh;
				q.s0 = ch.x0;
				q.t0 = ch.y0;
				q.s1 = ch.x1;
				q.t1 = ch.y1;
				q.advance = ch.xadvance * scale;
				quads.push_back(q);
			}

			if (ch.xadvance > 0.0f)
			{
				cursorX += ch.xadvance * scale;
			}
			else
			{
				cursorX += fontSize * 0.5f;
			}
		}
		return quads;
	}

	// ── legacy per-string texture (unchanged interface for compatibility) ────

	static void RasterizeGlyph(uint8_t* dest, int destW, int destH, int x, int y, const uint8_t* atlas, int atlasW,
							   int atlasH, const NativeFontChar& ch, int glyphW, int glyphH)
	{
		int srcX0 = static_cast<int>(ch.x0 * atlasW);
		int srcY0 = static_cast<int>(ch.y0 * atlasH);
		int srcX1 = static_cast<int>(ch.x1 * atlasW);
		int srcY1 = static_cast<int>(ch.y1 * atlasH);

		int srcW = srcX1 - srcX0;
		int srcH = srcY1 - srcY0;
		if (srcW <= 0 || srcH <= 0)
		{
			return;
		}

		for (int row = 0; row < glyphH && (y + row) < destH; ++row)
		{
			if (y + row < 0)
			{
				continue;
			}
			int srcRow = (row * srcH) / glyphH;
			for (int col = 0; col < glyphW && (x + col) < destW; ++col)
			{
				if (x + col < 0)
				{
					continue;
				}
				int srcCol = (col * srcW) / glyphW;

				int srcIdx = ((srcY0 + srcRow) * atlasW + (srcX0 + srcCol)) * 4;
				int dstIdx = ((y + row) * destW + (x + col)) * 4;

				dest[dstIdx + 3] = atlas[srcIdx + 3];
			}
		}
	}

	uint32_t TextRenderer::GetOrCreateTexture(const std::string& text, const NativeFont& font, float fontSize,
											  int padding)
	{
		if (text.empty())
		{
			m_LastWidth = 0;
			m_LastHeight = 0;
			return 0;
		}

		std::string key = text + "_" + std::to_string((int)fontSize);
		auto it = m_Cache.find(key);
		if (it != m_Cache.end())
		{
			m_LastWidth = it->second.width;
			m_LastHeight = it->second.height;
			return it->second.gpuTexture ? it->second.gpuTexture->GetNativeHandle() : 0;
		}

		float totalWidth = 0.0f;
		float maxHeight = 0.0f;
		std::vector<uint32_t> codepoints = DecodeUTF8(text);

		for (uint32_t cp : codepoints)
		{
			const auto& ch = font.GetChar(cp);
			if (ch.xadvance > 0.0f)
			{
				totalWidth += ch.xadvance * ((font.fontSize > 0.0f) ? (fontSize / font.fontSize) : 1.0f);
			}
			else
			{
				totalWidth += fontSize * 0.5f;
			}
			float glyphH =
				(ch.y1 - ch.y0) * font.atlasHeight * ((font.fontSize > 0.0f) ? (fontSize / font.fontSize) : 1.0f);
			if (glyphH > maxHeight)
			{
				maxHeight = glyphH;
			}
		}
		if (maxHeight <= 0.0f)
		{
			maxHeight = fontSize;
		}

		int width = std::max(1, static_cast<int>(totalWidth) + padding * 2);
		int height = std::max(1, static_cast<int>(maxHeight) + padding * 2);

		std::vector<uint8_t> pixels(width * height * 4);
		for (size_t i = 0; i < pixels.size(); i += 4)
		{
			pixels[i + 0] = 255;
			pixels[i + 1] = 255;
			pixels[i + 2] = 255;
			pixels[i + 3] = 0;
		}

		if (!font.atlasPixels.empty() && font.atlasWidth > 0 && font.atlasHeight > 0)
		{
			float scale = (font.fontSize > 0.0f) ? (fontSize / font.fontSize) : 1.0f;
			float cursorX = 0.0f;

			for (uint32_t cp : codepoints)
			{
				const auto& ch = font.GetChar(cp);

				int glyphW = static_cast<int>((ch.x1 - ch.x0) * font.atlasWidth * scale);
				int glyphH = static_cast<int>((ch.y1 - ch.y0) * font.atlasHeight * scale);
				if (glyphW <= 0 || glyphH <= 0)
				{
					cursorX += (ch.xadvance > 0.0f) ? ch.xadvance * scale : fontSize * 0.5f;
					continue;
				}

				int glyphX = padding + static_cast<int>(cursorX) + static_cast<int>(ch.xoff * scale);
				int glyphY = padding + static_cast<int>(fontSize) + static_cast<int>(ch.yoff * scale);

				RasterizeGlyph(pixels.data(), width, height, glyphX, glyphY, font.atlasPixels.data(), font.atlasWidth,
							   font.atlasHeight, ch, glyphW, glyphH);

				cursorX += ch.xadvance * scale;
			}
		}

		auto texture =
			Texture::Create(static_cast<uint32_t>(width), static_cast<uint32_t>(height), TextureFormat::RGBA8);
		if (texture)
		{
			texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
		}

		m_LastWidth = width;
		m_LastHeight = height;

		CachedText cached;
		cached.gpuTexture = texture;
		cached.width = width;
		cached.height = height;
		m_Cache[key] = cached;

		return texture ? texture->GetNativeHandle() : 0;
	}

} // namespace Chained
