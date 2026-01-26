#include "font_manager.h"
#include "engine/core/log.h"
#include "engine/render/asset_manager.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>

namespace CHEngine
{
std::unordered_map<std::string, ImFont *> FontManager::s_Fonts;
ImFont *FontManager::s_DefaultFont = nullptr;

void FontManager::Init()
{
    BuildFontAtlas();
}

void FontManager::Shutdown()
{
    s_Fonts.clear();
    s_DefaultFont = nullptr;
}

ImFont *FontManager::GetFont(FontType type, float size)
{
    int iSize = (int)size;
    std::string key =
        std::string(type == FontType::Bold ? "Bold" : "Regular") + "_" + std::to_string(iSize);
    if (s_Fonts.find(key) != s_Fonts.end())
        return s_Fonts[key];

    // Fallback: search for closest size in built-in fonts
    ImFont *best = s_DefaultFont;
    int minDiff = 1000;
    std::string prefix = (type == FontType::Bold ? "Bold_" : "Regular_");
    for (auto const &[name, font] : s_Fonts)
    {
        if (name.starts_with(prefix))
        {
            try
            {
                int fSize = std::stoi(name.substr(prefix.length()));
                int diff = std::abs(fSize - iSize);
                if (diff < minDiff)
                {
                    minDiff = diff;
                    best = font;
                }
            }
            catch (...)
            {
            }
        }
    }

    return best;
}

ImFont *FontManager::GetFont(const std::string &path, float size)
{
    if (path.empty())
        return GetFont(FontType::Regular, size);

    std::string key = path + "_" + std::to_string((int)size);
    if (s_Fonts.find(key) != s_Fonts.end())
        return s_Fonts[key];

    // Try to load on the fly if it's the first time
    if (LoadFont(path, size))
    {
        return s_Fonts[key];
    }

    return GetFont(FontType::Regular, size);
}

ImFont *FontManager::GetDefaultFont()
{
    return s_DefaultFont;
}

bool FontManager::LoadFont(const std::string &path, float size)
{
    ImGuiIO &io = ImGui::GetIO();
    std::string resolved = AssetManager::ResolvePath(path).string();

    if (!std::filesystem::exists(resolved))
    {
        CH_CORE_ERROR("FontManager: Failed to load font, file not found: {}", resolved);
        return false;
    }

    ImFont *font = io.Fonts->AddFontFromFileTTF(resolved.c_str(), size);
    if (!font)
    {
        CH_CORE_ERROR("FontManager: Failed to load font: {}", resolved);
        return false;
    }

    // IMPORTANT: After adding fonts to an ALREADY BUILT atlas,
    // ImGui usually requires rebuilding the texture and re-uploading it to GPU.
    // However, in our system, we prefer loading important fonts during Init.
    // Dynamic loading during render is risky but doable if rlImGui permits.

    s_Fonts[path + "_" + std::to_string((int)size)] = font;

    // Trigger atlas rebuild on next frame if possible?
    // For now, let's assume we use a dirty flag or re-upload.
    io.Fonts->Build();

    // Re-bind texture for rlImGui
    // rlImGuiReloadFonts(); // We might need this from rlImGui

    CH_CORE_INFO("FontManager: Dynamically loaded font: {} size {}", path, size);
    return true;
}

void FontManager::BuildFontAtlas()
{
    ImGuiIO &io = ImGui::GetIO();

    // 1. Clear existing fonts if needed (though usually Init is called once)
    io.Fonts->Clear();

    // 2. Load Professional Engine Fonts
    float sizes[] = {8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22,
                     24, 26, 28, 30, 32, 34, 36, 40, 44, 48, 56, 64, 72};

    std::string regularPath =
        AssetManager::ResolvePath("engine:font/lato/Lato-Regular.ttf").string();
    std::string boldPath = AssetManager::ResolvePath("engine:font/lato/Lato-Bold.ttf").string();
    std::string iconPath = AssetManager::ResolvePath("engine:font/fa-solid-900.ttf").string();

    bool regExists = std::filesystem::exists(regularPath);
    bool boldExists = std::filesystem::exists(boldPath);
    bool iconsExist = std::filesystem::exists(iconPath);

    // Default Fallback
    if (!regExists)
    {
        CH_CORE_WARN("FontManager: Primary font not found at {}. Using ImGui default.",
                     regularPath);
        s_DefaultFont = io.Fonts->AddFontDefault();
        return;
    }

    // Load multiple sizes into atlas
    for (float size : sizes)
    {
        // 1. Regular Font
        ImFontConfig reg_config;
        reg_config.FontDataOwnedByAtlas = true;
        ImFont *regFont = io.Fonts->AddFontFromFileTTF(regularPath.c_str(), size, &reg_config);

        // Merge Icons into Regular
        if (iconsExist)
        {
            static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            icons_config.GlyphOffset.y = 1.0f; // Slight adjustment for alignment
            io.Fonts->AddFontFromFileTTF(iconPath.c_str(), size, &icons_config, icons_ranges);
        }
        s_Fonts["Regular_" + std::to_string((int)size)] = regFont;

        if (size == 18.0f)
            s_DefaultFont = regFont;

        // 2. Bold Font
        if (boldExists)
        {
            ImFontConfig bold_config;
            ImFont *boldFont = io.Fonts->AddFontFromFileTTF(boldPath.c_str(), size, &bold_config);

            // Merge Icons into Bold too if needed
            if (iconsExist)
            {
                static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
                ImFontConfig icons_config;
                icons_config.MergeMode = true;
                icons_config.PixelSnapH = true;
                icons_config.GlyphOffset.y = 1.0f;
                io.Fonts->AddFontFromFileTTF(iconPath.c_str(), size, &icons_config, icons_ranges);
            }
            s_Fonts["Bold_" + std::to_string((int)size)] = boldFont;
        }
    }

    // Explicitly set default font for ImGui (Tabs, Menus, etc.)
    if (s_DefaultFont)
        io.FontDefault = s_DefaultFont;
    else if (!s_Fonts.empty())
        io.FontDefault = s_Fonts.begin()->second;

    CH_CORE_INFO("FontManager: Built font atlas with {} variations. Default font set.",
                 s_Fonts.size());

    // NOTE: This usually requires re-uploading the atlas to the GPU.
    // In our engine, Application/Window handles the backend setup correctly.
}
} // namespace CHEngine
