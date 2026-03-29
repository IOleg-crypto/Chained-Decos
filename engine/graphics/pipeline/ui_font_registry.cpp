#include "ui_font_registry.h"
#include "engine/core/base.h"
#include "engine/scene/project.h"
#include <cmath>
#include <filesystem>

namespace CHEngine
{

// Round font size to nearest 0.5 for cache key (avoids floating-point key mismatches)
static float RoundFontSize(float size)
{
    return std::round(size * 2.0f) / 2.0f;
}

std::string UIFontRegistry::MakeKey(const std::string& name, float size)
{
    // e.g. "fonts/Roboto-Regular.ttf|16.0"
    char buf[32];
    snprintf(buf, sizeof(buf), "|%.1f", RoundFontSize(size));
    return name + buf;
}

void UIFontRegistry::LoadProjectFonts()
{
    auto project = Project::GetActive();
    if (!project)
    {
        CH_CORE_WARN("UIFontRegistry: No active project, skipping font load.");
        return;
    }

    auto fontsDir = Project::GetAssetDirectory() / "fonts";
    if (!std::filesystem::exists(fontsDir))
    {
        CH_CORE_INFO("UIFontRegistry: No fonts directory found at '{}'", fontsDir.string());
        return;
    }

    const std::array<std::string, 2> validExtensions = {".ttf", ".otf"};
    int loaded = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(fontsDir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        auto ext = entry.path().extension().string();
        // Convert to lowercase for comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool validExt = false;
        for (const auto& ve : validExtensions)
        {
            if (ext == ve)
            {
                validExt = true;
                break;
            }
        }
        if (!validExt)
        {
            continue;
        }

        // relative key: "fonts/Roboto-Regular.ttf"
        auto relativePath = std::filesystem::relative(entry.path(), Project::GetAssetDirectory());
        std::string relKey = relativePath.generic_string(); // forward slashes

        m_KnownPaths[relKey] = entry.path().string();
        CH_CORE_INFO("UIFontRegistry: Discovered font '{}'", relKey);
        loaded++;
    }

    CH_CORE_INFO("UIFontRegistry: Discovered {} font file(s) in '{}'", loaded, fontsDir.string());
}

ImFont* UIFontRegistry::GetFont(const std::string& relativeName, float pixelSize) const
{
    if (relativeName.empty() || relativeName == "Default")
    {
        return nullptr; // caller should use ImGui default
    }

    float size = (pixelSize > 0.0f) ? pixelSize : 16.0f;
    std::string key = MakeKey(relativeName, size);

    // Fast path: already in atlas
    auto it = m_Fonts.find(key);
    if (it != m_Fonts.end())
    {
        return it->second;
    }

    // Slow path: need to register for this size.
    // NOTE: This only works before ImGui font atlas is built (before first frame).
    // At runtime this returns nullptr — font must be pre-registered.
    auto pathIt = m_KnownPaths.find(relativeName);
    if (pathIt == m_KnownPaths.end())
    {
        CH_CORE_WARN("UIFontRegistry: Font '{}' not discovered. Check assets/fonts/ directory.", relativeName);
        return nullptr;
    }

    // Cast away const — lazy loading into atlas (only valid pre-build)
    return const_cast<UIFontRegistry*>(this)->RegisterFont(relativeName, pathIt->second, size);
}

ImFont* UIFontRegistry::RegisterFont(const std::string& relativeName, const std::string& absolutePath, float pixelSize)
{
    std::string key = MakeKey(relativeName, pixelSize);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(absolutePath.c_str(), pixelSize);
    if (!font)
    {
        CH_CORE_ERROR("UIFontRegistry: Failed to load font '{}' at size {:.1f}", absolutePath, pixelSize);
        return nullptr;
    }

    m_Fonts[key] = font;

    if (!m_DefaultFont)
    {
        m_DefaultFont = font;
    }

    CH_CORE_INFO("UIFontRegistry: Loaded '{}' at {:.1f}px", relativeName, pixelSize);
    return font;
}

void UIFontRegistry::Clear()
{
    m_Fonts.clear();
    m_KnownPaths.clear();
    m_DefaultFont = nullptr;
}

} // namespace CHEngine
