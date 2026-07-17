#ifndef CH_UI_FONT_REGISTRY_H
#define CH_UI_FONT_REGISTRY_H

#include "imgui.h"
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace Chained
{

// Manages ImGui font atlas entries for UI rendering.
// Fonts must be registered before BuildAtlas() is called (before the first frame).
// Typical usage: call LoadProjectFonts() between ImGui::CreateContext() and the first frame.
class UIFontRegistry
{
public:
    UIFontRegistry()  = default;
    ~UIFontRegistry() = default;

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
    ImFont* GetDefaultFont() const { return m_DefaultFont; }

    // True if any fonts were successfully loaded.
    bool HasFonts() const { return !m_Fonts.empty(); }

    // Clears all registered fonts (call before re-loading a new project).
    void Clear();

private:
    // Registers a single TTF/OTF file at the given absolute path under a relative name key.
    // Returns the loaded ImFont* or nullptr on failure.
    ImFont* RegisterFont(const std::string& relativeName, const std::string& absolutePath, float pixelSize);

    // Build a cache key from name + size (rounded to 0.5px increments).
    static std::string MakeKey(const std::string& name, float size);
    static std::string NormalizeFontName(std::string name);

    // map<"fonts/Roboto.ttf|16.0" -> ImFont*>
    std::unordered_map<std::string, ImFont*> m_Fonts;

    // Tracks which relative paths have been discovered (name -> absolute path)
    std::unordered_map<std::string, std::string> m_KnownPaths;

    ImFont* m_DefaultFont = nullptr;
};

} // namespace Chained

#endif // CH_UI_FONT_REGISTRY_H
