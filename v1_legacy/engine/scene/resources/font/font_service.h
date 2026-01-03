#ifndef CD_SCENE_RESOURCES_FONT_FONT_SERVICE_H
#define CD_SCENE_RESOURCES_FONT_FONT_SERVICE_H

#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class FontService
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    // Load a font from file and cache it
    static bool LoadFont(const std::string &name, const std::string &path);

    // Get a cached font. Returns default font if not found.
    static Font GetFont(const std::string &name);

    ~FontService() = default;

private:
    FontService() = default;

    bool InternalLoadFont(const std::string &name, const std::string &path);
    Font InternalGetFont(const std::string &name);
    void InternalShutdown();

private:
    std::unordered_map<std::string, Font> m_fonts;
};
} // namespace CHEngine

#endif // CD_SCENE_RESOURCES_FONT_FONT_SERVICE_H
