#ifndef CH_FONT_CHOICE_H
#define CH_FONT_CHOICE_H
#include <string>
#include <vector>
#include <filesystem>
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"

namespace Chained
{

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
const std::vector<FontChoice>& GetEditorFontChoices();
} // namespace Chained

#endif /* CH_FONT_CHOICE_H */
