#ifndef CH_FONT_CHOICE_H
#define CH_FONT_CHOICE_H
#include <string>
#include <vector>
#include <filesystem>
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"

namespace Chained{

struct FontChoice
{
    std::string Label; // e.g. "Lato Bold" (derived from filename)
    std::string Path;  // relative to the engine root, e.g. "resources/font/lato/lato-bold.ttf"
};

// Turns "lato-bold" / "AlanSans_Medium" into "Lato Bold" / "AlanSans Medium".
inline std::string MakeFontLabel(const std::filesystem::path& file)
{
    std::string stem = file.stem().string();
    std::string label;
    label.reserve(stem.size());
    bool upperNext = true;
    for (char c : stem)
    {
        if (c == '-' || c == '_')
        {
            label += ' ';
            upperNext = true;
        }
        else
        {
            label += upperNext ? (char)std::toupper((unsigned char)c) : c;
            upperNext = false;
        }
    }
    return label;
}

// Scans <EngineRoot>/resources/font recursively; cached after the first call.
// FontAwesome icon fonts are excluded — merging them as the main UI font breaks text.
inline const std::vector<FontChoice>& GetEditorFontChoices()
{
    static std::vector<FontChoice> s_Choices;
    static bool s_Scanned = false;
    if (s_Scanned)
    {
        return s_Choices;
    }
    s_Scanned = true;

    auto* assetManager = ServiceLocator::TryGet<AssetManager>();
    if (!assetManager)
    {
        CH_CORE_WARN("EditorGUI: AssetManager not available; font picker will be empty.");
        return s_Choices;
    }
    auto engineRoot = assetManager->GetEngineRoot();
    const std::filesystem::path fontDir = engineRoot / "resources" / "font";

    std::error_code ec;
    if (!std::filesystem::exists(fontDir, ec) || ec)
    {
        CH_CORE_WARN("EditorGUI: Font directory '{}' not found; font picker will be empty.", fontDir.string());
        return s_Choices;
    }

    for (std::filesystem::recursive_directory_iterator
             it(fontDir, std::filesystem::directory_options::skip_permission_denied, ec),end;  it != end && !ec; it.increment(ec))
    {
        if (!it->is_regular_file(ec))
        {
            ec.clear();
            continue;
        }

        std::string ext = it->path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (ext != ".ttf" && ext != ".otf")
        {
            continue;
        }

        // Skip icon fonts (merged separately in LoadEditorFonts).
        std::string stem = it->path().stem().string();
        if (stem.rfind("fa-", 0) == 0)
        {
            continue;
        }

        auto rel = std::filesystem::relative(it->path(), engineRoot, ec);
        if (ec)
        {
            ec.clear();
            continue;
        }

        s_Choices.push_back({MakeFontLabel(it->path()), rel.generic_string()});
    }

    std::sort(s_Choices.begin(), s_Choices.end(),
              [](const FontChoice& a, const FontChoice& b) { return a.Label < b.Label; });

    CH_CORE_INFO("EditorGUI: Discovered {} editor font(s) in '{}'.", s_Choices.size(), fontDir.string());
    return s_Choices;
}
}; // namespace

#endif /* CH_FONT_CHOICE_H */
