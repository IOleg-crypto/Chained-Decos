#include "FontService.h"
#include <iostream>

namespace CHEngine
{
bool FontService::LoadFont(const std::string &name, const std::string &path)
{
    if (m_fonts.find(name) != m_fonts.end())
        return true;

    Font font = ::LoadFont(path.c_str());
    if (font.texture.id == 0)
    {
        std::cerr << "[FontService] Failed to load font: " << name << " from " << path << "\n";
        return false;
    }

    m_fonts[name] = font;
    std::cout << "[FontService] Successfully loaded font: " << name << "\n";
    return true;
}

Font FontService::GetFont(const std::string &name)
{
    auto it = m_fonts.find(name);
    if (it != m_fonts.end())
        return it->second;

    return ::GetFontDefault();
}

void FontService::Shutdown()
{
    for (auto &pair : m_fonts)
    {
        ::UnloadFont(pair.second);
    }
    m_fonts.clear();
}
} // namespace CHEngine
