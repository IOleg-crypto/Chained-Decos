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

		uint32_t GetOrCreateTexture(const std::string& text, const NativeFont& font, float fontSize = 32.0f,
									int padding = 4);

		int GetLastWidth() const
		{
			return m_LastWidth;
		}
		int GetLastHeight() const
		{
			return m_LastHeight;
		}

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
