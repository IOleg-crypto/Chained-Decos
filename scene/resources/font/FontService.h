#ifndef FONT_SERVICE_H
#define FONT_SERVICE_H

#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class FontService
{
public:
    FontService() = default;
    ~FontService() = default;

    // Load a font from file and cache it
    bool LoadFont(const std::string &name, const std::string &path);

    // Get a cached font. Returns default font if not found.
    Font GetFont(const std::string &name);

    // Unload all fonts
    void Shutdown();

private:
    std::unordered_map<std::string, Font> m_fonts;
};
} // namespace CHEngine

#endif // FONT_SERVICE_H
