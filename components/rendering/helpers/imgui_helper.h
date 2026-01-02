#ifndef CD_COMPONENTS_RENDERING_HELPERS_IMGUI_HELPER_H
#define CD_COMPONENTS_RENDERING_HELPERS_IMGUI_HELPER_H

#include <string>

// Utility class for ImGui operations
class ImGuiHelper
{
public:
    // Initialize ImGui font from file
    // Returns true if font was loaded successfully, false otherwise
    static bool InitializeFont(const std::string& fontPath, float fontSize);

    // Check if font file exists
    static bool FontFileExists(const std::string& fontPath);
};

#endif // CD_COMPONENTS_RENDERING_HELPERS_IMGUI_HELPER_H





