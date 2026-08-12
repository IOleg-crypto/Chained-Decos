#include "engine/graphics/text_renderer.h"
#include "engine/graphics/api/texture.h"
#include <vector>
#include <algorithm>
#include <cstring>

namespace Chained
{

	void TextRenderer::Init()
	{
		m_Initialized = true;
	}

	void TextRenderer::Shutdown()
	{
		m_Cache.clear();
		m_Initialized = false;
	}

	static void RasterizeGlyph(uint8_t* dest, int destW, int destH, int x, int y, const uint8_t* atlas, int atlasW,
							   int atlasH, const NativeFontChar& ch, int glyphW, int glyphH)
	{
		// UV -> pixel coordinates in atlas
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

				// Atlas is white RGB with alpha — copy alpha into dest (which starts as white)
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

		float scale = (font.fontSize > 0.0f) ? (fontSize / font.fontSize) : 1.0f;

		// Calculate total width and max height
		float totalWidth = 0.0f;
		float maxHeight = 0.0f;

		for (char c : text)
		{
			unsigned char uc = static_cast<unsigned char>(c);
			if (uc >= 128)
			{
				uc = '?';
			}

			const auto& ch = font.chars[uc];
			if (ch.xadvance > 0.0f)
			{
				totalWidth += ch.xadvance * scale;
			}
			else
			{
				totalWidth += fontSize * 0.5f;
			}

			float glyphH = (ch.y1 - ch.y0) * font.atlasHeight * scale;
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

		// Create white RGBA texture
		std::vector<uint8_t> pixels(width * height * 4, 255);

		// Rasterize glyphs if atlas data is available
		if (!font.atlasPixels.empty() && font.atlasWidth > 0 && font.atlasHeight > 0)
		{
			float cursorX = 0.0f;

			for (char c : text)
			{
				unsigned char uc = static_cast<unsigned char>(c);
				if (uc >= 128)
				{
					uc = '?';
				}

				const auto& ch = font.chars[uc];

				int glyphW = static_cast<int>((ch.x1 - ch.x0) * font.atlasWidth * scale);
				int glyphH = static_cast<int>((ch.y1 - ch.y0) * font.atlasHeight * scale);
				if (glyphW <= 0 || glyphH <= 0)
				{
					cursorX += (ch.xadvance > 0.0f) ? ch.xadvance * scale : fontSize * 0.5f;
					continue;
				}

				// yoff is the offset from the baseline; place glyph so baseline sits at fontSize
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
