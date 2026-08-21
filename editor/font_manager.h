#ifndef CH_FONT_MANAGER_H
#define CH_FONT_MANAGER_H

#include "editor/project/editor_settings.h"

namespace Chained
{

	class FontManager
	{
	public:
		explicit FontManager(EditorConfig& config);

		void LoadFonts();
		void ReloadFonts();
		void AddFontsToAtlas();
		void RequestReload();

		bool HasPendingReload() const
		{
			return m_PendingReload;
		}
		void ClearPendingReload()
		{
			m_PendingReload = false;
		}

	private:
		EditorConfig& m_Config;
		bool m_PendingReload = false;
	};

} // namespace Chained

#endif // CH_FONT_MANAGER_H
