//
// Created by AI Assistant
// ImGuiHelper - Utility functions for ImGui initialization and management
// Separated from RenderManager to follow Single Responsibility Principle
//

#ifndef IMGUI_HELPER_H
#define IMGUI_HELPER_H

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

#endif // IMGUI_HELPER_H

