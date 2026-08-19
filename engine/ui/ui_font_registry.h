#ifndef CH_UI_FONT_REGISTRY_H
#define CH_UI_FONT_REGISTRY_H

#include "engine/core/service.h"
#include "imgui.h"
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Chained
{

	// Manages ImGui font atlas entries for UI rendering.
	// Fonts must be registered before BuildAtlas() is called (before the first frame).
	// Typical usage: call LoadProjectFonts() between ImGui::CreateContext() and the first frame.
	class UIFontRegistry : public Service
	{
	public:
		UIFontRegistry() = default;
		~UIFontRegistry() override = default;

		void Initialize() override
		{
		}
		void Shutdown() override
		{
		}

		// Scans <ProjectAssetDir>/fonts/ for all TTF/OTF files and discovers paths.
		// FontName key = relative path from assets/, e.g. "fonts/Roboto-Regular.ttf".
		// Multiple sizes are loaded explicitly via PreloadFonts().
		void LoadProjectFonts();

		// Preloads explicit font tuples into ImGui atlas.
		// Returns number of newly registered tuples.
		int PreloadFonts(const std::vector<std::pair<std::string, float>>& requests, bool allowRuntimeMutation);

		// Ensures project default font is available and returns it.
		// Falls back to nullptr if no project font files are found.
		ImFont* EnsureDefaultProjectFont(float pixelSize, bool allowRuntimeMutation);

		// Returns the ImFont* for the given relative font name and pixel size.
		// If not found or not loaded, returns nullptr (ImGui will use its default font).
		// Non-const version allows lazy registration of fonts discovered but not yet loaded.
		ImFont* GetFont(const std::string& relativeName, float pixelSize);

		// Const version — returns nullptr if font is not yet loaded (no lazy registration).
		const ImFont* GetFont(const std::string& relativeName, float pixelSize) const;

		// Returns the default font (first registered, or ImGui built-in default).
		ImFont* GetDefaultFont() const
		{
			return m_DefaultFont;
		}

		// True if any fonts were successfully loaded.
		bool HasFonts() const
		{
			return !m_Fonts.empty();
		}

		// Returns all discovered font names (relative paths from assets/).
		std::vector<std::string> GetKnownFontNames() const;

		// Clears all registered fonts (call before re-loading a new project).
		void Clear();

		// Returns true if fonts were registered after the first frame and the GPU
		// atlas texture must be rebuilt via ImGuiLayer::RefreshFontAtlasTexture().
		bool NeedsAtlasRebuild() const
		{
			return m_NeedsRebuild;
		}

		// Clears the rebuild flag (call after the atlas GPU texture has been refreshed).
		void ClearRebuildFlag()
		{
			m_NeedsRebuild = false;
		}

	private:
		// Registers a single TTF/OTF file at the given absolute path under a relative name key.
		// Returns the loaded ImFont* or nullptr on failure (failures are cached as nullptr).
		ImFont* RegisterFont(const std::string& relativeName, const std::string& absolutePath, float pixelSize);

		// Finalises font registration: stores in m_Fonts, sets m_DefaultFont, flags atlas rebuild.
		ImFont* CommitFont(const std::string& normalizedName, ImFont* font);

		// Registers relKey → path in m_KnownPaths, increments discovered, and adds font/↔fonts/ alias.
		void RegisterKnownFont(const std::string& relKey, const std::string& path, int& discovered);

		static std::string NormalizeFontName(std::string name);

		// Keyed by normalized relative name, e.g. "fonts/Roboto.ttf". One ImFont* per
		// file — with ImGui 1.92 dynamic fonts any render size works from a single entry
		// (pass the size to PushFont()/ImDrawList::AddText()).
		std::unordered_map<std::string, ImFont*> m_Fonts;

		// Failed font names are tracked separately so the registry can retry pack/file loads
		// instead of permanently caching a null font pointer from a transient failure.
		std::unordered_set<std::string> m_FailedFonts;

		// Tracks which relative paths have been discovered (name -> absolute path)
		std::unordered_map<std::string, std::string> m_KnownPaths;

		ImFont* m_DefaultFont = nullptr;
		bool m_NeedsRebuild = false;

		// Raw TTF/OTF byte buffers for fonts read from pack.
		// FontDataOwnedByAtlas = false, so these must outlive the atlas build.
		std::unordered_map<std::string, std::vector<uint8_t>> m_PackFontData;
	};

} // namespace Chained

#endif // CH_UI_FONT_REGISTRY_H
