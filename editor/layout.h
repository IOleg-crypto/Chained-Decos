#ifndef CH_EDITOR_LAYOUT_H
#define CH_EDITOR_LAYOUT_H

#include "editor/panels.h"
#include <string>
#include <vector>

namespace Chained
{

	class EditorLayout
	{
	public:
		EditorLayout(EditorPanels& panels);

		void ResetLayout();

		void LoadPreset(const std::string& filepath);
		void SaveCurrent(const std::string& filepath);
		void SaveDefaultLayout();

		// Named preset management
		void SavePreset(const std::string& name);
		void LoadPresetByName(const std::string& name);
		void DeletePreset(const std::string& name);
		std::vector<std::string> GetPresetNames() const;
		const std::string& GetActivePreset() const
		{
			return m_ActivePreset;
		}

		void OnImGuiRender();

	private:
		std::string GetPresetDirectory() const;
		std::string GetPresetPath(const std::string& name) const;

		EditorPanels& m_Panels;
		uint32_t m_DockSpaceID = 0;
		bool m_NeedsRebuild = true;
		std::string m_ActivePreset = "Default";
	};

} // namespace Chained

#endif // CH_EDITOR_LAYOUT_H
