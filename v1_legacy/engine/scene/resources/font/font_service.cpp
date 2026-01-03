#include "font_service.h"
#include <iostream>
#include <memory>

namespace CHEngine
{
static std::unique_ptr<FontService> s_Instance = nullptr;

void FontService::Init()
{
    s_Instance = std::unique_ptr<FontService>(new FontService());
}

bool FontService::IsInitialized()
{
    return s_Instance != nullptr;
}

void FontService::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdown();
        s_Instance.reset();
    }
}

bool FontService::LoadFont(const std::string &name, const std::string &path)
{
    return s_Instance->InternalLoadFont(name, path);
}

Font FontService::GetFont(const std::string &name)
{
    return s_Instance->InternalGetFont(name);
}

bool FontService::InternalLoadFont(const std::string &name, const std::string &path)
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

Font FontService::InternalGetFont(const std::string &name)
{
    auto it = m_fonts.find(name);
    if (it != m_fonts.end())
        return it->second;

    return ::GetFontDefault();
}

void FontService::InternalShutdown()
{
    for (auto &pair : m_fonts)
    {
        ::UnloadFont(pair.second);
    }
    m_fonts.clear();
}
} // namespace CHEngine
