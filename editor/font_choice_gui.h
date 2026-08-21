#ifndef CH_FONT_CHOICE_H
#define CH_FONT_CHOICE_H
#include <string>
#include <vector>
#include <filesystem>

namespace Chained
{

	struct FontChoice
	{
		std::string Label; // e.g. "Lato Bold" (derived from filename)
		std::string Path;  // relative to the engine root, prefixed with "engine/", e.g.
						   // "engine/resources/font/lato/lato-bold.ttf"
	};

	// Turns "lato-bold" / "AlanSans_Medium" into "Lato Bold" / "AlanSans Medium".
	std::string MakeFontLabel(const std::filesystem::path& file);

	// Scans <EngineRoot>/resources/font recursively; cached after the first call.
	// FontAwesome icon fonts are excluded — merging them as the main UI font breaks text.
	const std::vector<FontChoice>& GetEditorFontChoices();
} // namespace Chained

#endif /* CH_FONT_CHOICE_H */
