#include "ui_font_registry.h"
#include "engine/common/asset_path.h"
#include "engine/common/base.h"
#include "engine/project/project.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace Chained
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

std::string UIFontRegistry::NormalizeFontName(std::string name)
{
    return NormalizeAssetPath(name);
}

void UIFontRegistry::LoadProjectFonts()
{
    m_KnownPaths.clear();

    if (!Project::GetActive())
    {
        CH_CORE_WARN("UIFontRegistry: No active project, skipping font load.");
        return;
    }

    const std::filesystem::path assetDir = Project::GetAssetDirectory();
    const std::array<std::filesystem::path, 2> candidateDirs = {
        assetDir / "fonts",
        assetDir / "font",
    };

    const std::array<std::string, 2> validExtensions = {".ttf", ".otf"};
    int discovered = 0;

    for (const auto& fontsDir : candidateDirs)
    {
        if (!FileExists(fontsDir))
        {
            continue;
        }

        std::error_code iterError;
        std::filesystem::recursive_directory_iterator it(
            fontsDir,
            std::filesystem::directory_options::skip_permission_denied,
            iterError);
        std::filesystem::recursive_directory_iterator end;

        if (iterError)
        {
            CH_CORE_WARN("UIFontRegistry: Failed to scan '{}' ({})", fontsDir.string(), iterError.message());
            continue;
        }

        for (; it != end; it.increment(iterError))
        {
            if (iterError)
            {
                CH_CORE_WARN("UIFontRegistry: Iteration failed in '{}' ({})", fontsDir.string(), iterError.message());
                iterError.clear();
                continue;
            }

            const auto& entry = *it;
            if (!entry.is_regular_file(iterError) || iterError)
            {
                iterError.clear();
                continue;
            }

            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

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

            auto relativePath = std::filesystem::relative(entry.path(), assetDir, iterError);
            if (iterError)
            {
                CH_CORE_WARN("UIFontRegistry: Failed to build relative path for '{}' ({})",
                             entry.path().string(),
                             iterError.message());
                iterError.clear();
                continue;
            }

            std::string relKey = NormalizeFontName(relativePath.generic_string());
            std::string absPath = entry.path().string();

            if (m_KnownPaths.emplace(relKey, absPath).second)
            {
                CH_CORE_INFO("UIFontRegistry: Discovered font '{}'", relKey);
                discovered++;
            }

            // Compatibility alias between legacy "font/" and "fonts/" prefixes.
            if (relKey.rfind("font/", 0) == 0)
            {
                m_KnownPaths.emplace("fonts/" + relKey.substr(5), absPath);
            }
            else if (relKey.rfind("fonts/", 0) == 0)
            {
                m_KnownPaths.emplace("font/" + relKey.substr(6), absPath);
            }
        }
    }

    if (discovered == 0)
    {
        CH_CORE_INFO("UIFontRegistry: No project fonts found in '{}' or '{}'.",
                     (assetDir / "font").string(),
                     (assetDir / "fonts").string());
        return;
    }

    CH_CORE_INFO("UIFontRegistry: Discovered {} font file(s).", discovered);
}

ImFont* UIFontRegistry::EnsureDefaultProjectFont(float pixelSize, bool allowRuntimeMutation)
{
    const float size = (pixelSize > 0.0f) ? pixelSize : 16.0f;

    static const std::array<const char*, 8> preferredFonts = {
        "font/alan_sans/static/alansans-regular.ttf",
        "font/alan_sans/static/alansans-medium.ttf",
        "fonts/alan_sans/static/alansans-regular.ttf",
        "fonts/alan_sans/static/alansans-medium.ttf",
        "font/lato/lato-regular.ttf",
        "font/lato/lato-bold.ttf",
        "fonts/lato/lato-regular.ttf",
        "fonts/lato/lato-bold.ttf",
    };

    std::vector<std::pair<std::string, float>> preloadRequests;
    for (const char* preferred : preferredFonts)
    {
        const std::string normalized = NormalizeFontName(preferred);
        if (m_KnownPaths.find(normalized) != m_KnownPaths.end())
        {
            preloadRequests.emplace_back(normalized, size);
            break;
        }
    }

    if (preloadRequests.empty() && !m_KnownPaths.empty())
    {
        std::string fallbackName;
        for (const auto& [name, _] : m_KnownPaths)
        {
            if (name.rfind("font/", 0) == 0 || name.rfind("fonts/", 0) == 0)
            {
                fallbackName = name;
                break;
            }
        }

        if (!fallbackName.empty())
        {
            preloadRequests.emplace_back(fallbackName, size);
        }
    }

    if (preloadRequests.empty())
    {
        return nullptr;
    }

    PreloadFonts(preloadRequests, allowRuntimeMutation);

    for (const auto& [name, requestedSize] : preloadRequests)
    {
        ImFont* font = GetFont(name, requestedSize);
        if (font)
        {
            return font;
        }
    }

    return nullptr;
}

ImFont* UIFontRegistry::GetFont(const std::string& relativeName, float pixelSize) const
{
    const std::string normalizedName = NormalizeFontName(relativeName);
    if (normalizedName.empty() || normalizedName == "Default")
    {
        return nullptr; // caller should use ImGui default
    }

    const float size = (pixelSize > 0.0f) ? pixelSize : 16.0f;
    const std::string key = MakeKey(normalizedName, size);

    // Avoid mutating ImGui font atlas at runtime. If a custom font wasn't preloaded
    // before the first frame, caller should gracefully fallback to default font.
    if (ImGui::GetFrameCount() > 0)
    {
        auto it = m_Fonts.find(key);
        if (it != m_Fonts.end())
        {
            return it->second;
        }
        return nullptr;
    }

    // Fast path: already in atlas
    auto it = m_Fonts.find(key);
    if (it != m_Fonts.end())
    {
        return it->second;
    }

    // Slow path: need to register for this size.
    // NOTE: This only works before ImGui font atlas is built (before first frame).
    // At runtime this returns nullptr — font must be pre-registered.
    auto pathIt = m_KnownPaths.find(normalizedName);
    if (pathIt != m_KnownPaths.end())
    {
        // Cast away const — lazy loading into atlas (only valid pre-build)
        return const_cast<UIFontRegistry*>(this)->RegisterFont(normalizedName, pathIt->second, size);
    }

    std::filesystem::path directPath(normalizedName);
    if (directPath.is_absolute() && FileExists(directPath))
    {
        return const_cast<UIFontRegistry*>(this)->RegisterFont(normalizedName, directPath.string(), size);
    }

    CH_CORE_WARN("UIFontRegistry: Font '{}' not discovered. Check assets/font or assets/fonts.", normalizedName);
    return nullptr;
}

int UIFontRegistry::PreloadFonts(const std::vector<std::pair<std::string, float>>& requests, bool allowRuntimeMutation)
{
    if (requests.empty())
    {
        return 0;
    }

    if (ImGui::GetFrameCount() > 0 && !allowRuntimeMutation)
    {
        CH_CORE_WARN("UIFontRegistry: Ignoring preload request after first frame (runtime mutation disabled).");
        return 0;
    }

    int loaded = 0;
    std::unordered_set<std::string> seenKeys;
    std::unordered_set<std::string> missingFonts;

    for (const auto& [fontNameRaw, pixelSizeRaw] : requests)
    {
        const std::string fontName = NormalizeFontName(fontNameRaw);
        if (fontName.empty() || fontName == "Default")
        {
            continue;
        }

        const float pixelSize = (pixelSizeRaw > 0.0f) ? pixelSizeRaw : 16.0f;
        const std::string key = MakeKey(fontName, pixelSize);
        if (!seenKeys.insert(key).second)
        {
            continue;
        }

        if (m_Fonts.find(key) != m_Fonts.end())
        {
            continue;
        }

        std::string absolutePath;
        auto knownPathIt = m_KnownPaths.find(fontName);
        if (knownPathIt != m_KnownPaths.end())
        {
            absolutePath = knownPathIt->second;
        }
        else
        {
            std::filesystem::path directPath(fontName);
            if (directPath.is_absolute() && FileExists(directPath))
            {
                absolutePath = directPath.string();
            }
        }

        if (absolutePath.empty())
        {
            missingFonts.insert(fontName);
            continue;
        }

        if (RegisterFont(fontName, absolutePath, pixelSize))
        {
            loaded++;
        }
    }

    for (const auto& fontName : missingFonts)
    {
        CH_CORE_WARN("UIFontRegistry: Missing font '{}' requested by runtime scene.", fontName);
    }

    return loaded;
}

ImFont* UIFontRegistry::RegisterFont(const std::string& relativeName, const std::string& absolutePath, float pixelSize)
{
    const std::string normalizedName = NormalizeFontName(relativeName);
    if (normalizedName.empty())
    {
        return nullptr;
    }

    std::string key = MakeKey(normalizedName, pixelSize);

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

    CH_CORE_INFO("UIFontRegistry: Loaded '{}' at {:.1f}px", normalizedName, pixelSize);
    return font;
}

void UIFontRegistry::Clear()
{
    m_Fonts.clear();
    m_KnownPaths.clear();
    m_DefaultFont = nullptr;
}

} // namespace Chained
