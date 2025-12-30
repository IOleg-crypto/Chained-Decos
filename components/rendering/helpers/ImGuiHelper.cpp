#include "ImGuiHelper.h"
#include "core/Log.h"
#include <imgui.h>
#include <raylib.h>

bool ImGuiHelper::InitializeFont(const std::string &fontPath, float fontSize)
{
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    // Check if font file exists first
    if (FileExists(fontPath.c_str()))
    {
        // Try to load font for ImGui
        ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
        if (font != nullptr)
        {
            // Don't build here - will be built in BeginFrame() after BeginDrawing()
            CD_CORE_INFO("Font loaded for ImGui: %s (%.1fpx) (will be built on first frame)",
                         fontPath.c_str(), fontSize);
            return true;
        }
        else
        {
            CD_CORE_WARN("Failed to load font for ImGui: %s, using default ImGui font",
                         fontPath.c_str());
        }
    }
    else
    {
        CD_CORE_WARN("Font file not found: %s, using default ImGui font", fontPath.c_str());
    }

    // Add default font as fallback
    io.Fonts->AddFontDefault();
    // Don't build here - will be built in BeginFrame() after BeginDrawing()
    return false;
}

bool ImGuiHelper::FontFileExists(const std::string &fontPath)
{
    return FileExists(fontPath.c_str());
}
