#ifndef FONT_SERVICE_H
#define FONT_SERVICE_H

#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>


namespace ChainedDecos
{
class FontService
{
public:
    static FontService &Get()
    {
        static FontService instance;
        return instance;
    }

    // Load a font from file and cache it
    bool LoadFont(const std::string &name, const std::string &path);

    // Get a cached font. Returns default font if not found.
    Font GetFont(const std::string &name);

    // Unload all fonts
    void Shutdown();

private:
    FontService() = default;
    ~FontService() = default;

    std::unordered_map<std::string, Font> m_fonts;
};
} // namespace ChainedDecos

#endif // FONT_SERVICE_H
