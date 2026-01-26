#ifndef CH_FONT_MANAGER_H
#define CH_FONT_MANAGER_H

#include "engine/core/base.h"
#include <imgui.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
enum class FontType
{
    Regular,
    Bold,
    Italic
};

class FontManager
{
public:
    static void Init();
    static void Shutdown();

    static ImFont *GetFont(FontType type, float size);
    static ImFont *GetFont(const std::string &path, float size);
    static ImFont *GetDefaultFont();

    static bool LoadFont(const std::string &path, float size);

private:
    static void BuildFontAtlas();

    static std::unordered_map<std::string, ImFont *> s_Fonts;
    static ImFont *s_DefaultFont;
};
} // namespace CHEngine

#endif // CH_FONT_MANAGER_H
