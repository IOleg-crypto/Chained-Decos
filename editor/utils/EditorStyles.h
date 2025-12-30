#ifndef EDITOR_STYLES_H
#define EDITOR_STYLES_H

#include <imgui.h>

namespace EditorStyles
{
// Font indices - must match order fonts are added in EditorApplication.cpp
enum Font : unsigned char
{
    FONT_REGULAR = 0, // 14px - body text
    FONT_BOLD = 1,    // 16px - labels, headers
    FONT_TITLE = 2    // 18px - panel titles
};

// Get font by index - use in ImGui::PushFont(GetFont(FONT_BOLD))
inline ImFont *GetFont(Font font)
{
    ImGuiIO &io = ImGui::GetIO();
    if (font < io.Fonts->Fonts.Size)
        return io.Fonts->Fonts[font];
    return io.Fonts->Fonts[0]; // Fallback to default
}

// Helper colors for consistent styling
namespace Colors
{
// Accent colors
inline ImVec4 AccentBlue()
{
    return ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
}
inline ImVec4 AccentGreen()
{
    return ImVec4(0.35f, 0.78f, 0.35f, 1.00f);
}
inline ImVec4 AccentRed()
{
    return ImVec4(0.90f, 0.35f, 0.35f, 1.00f);
}
inline ImVec4 AccentYellow()
{
    return ImVec4(0.95f, 0.80f, 0.25f, 1.00f);
}
inline ImVec4 AccentOrange()
{
    return ImVec4(0.95f, 0.55f, 0.20f, 1.00f);
}

// For vector components (Unity-style X/Y/Z colors)
inline ImVec4 VectorX()
{
    return ImVec4(0.85f, 0.25f, 0.25f, 1.00f);
} // Red
inline ImVec4 VectorY()
{
    return ImVec4(0.45f, 0.75f, 0.25f, 1.00f);
} // Green
inline ImVec4 VectorZ()
{
    return ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
} // Blue
} // namespace Colors

inline void ApplyDarkTheme()
{
    auto &style = ImGui::GetStyle();
    auto &colors = style.Colors;

    // Visual properties - Modern rounded look
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.SeparatorTextBorderSize = 1.0f;

    // Spacing - comfortable padding
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    // Colors - Modern dark theme (Unreal Engine / Unity Pro inspired)
    ImVec4 bg_dark = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);      // Very dark background
    ImVec4 bg_mid = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);       // Panel background
    ImVec4 bg_light = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);     // Input fields
    ImVec4 border = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);       // Border color
    ImVec4 accent = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);       // Vibrant blue accent
    ImVec4 accent_hover = ImVec4(0.38f, 0.68f, 1.00f, 1.00f); // Brighter on hover
    ImVec4 accent_dim = ImVec4(0.20f, 0.45f, 0.75f, 1.00f);   // Darker accent

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Window backgrounds
    colors[ImGuiCol_WindowBg] = bg_mid;
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);

    // Borders
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);

    // Frame (input fields, etc)
    colors[ImGuiCol_FrameBg] = bg_light;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);

    // Title bar
    colors[ImGuiCol_TitleBg] = bg_dark;
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = bg_dark;

    // Menu bar
    colors[ImGuiCol_MenuBarBg] = bg_dark;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.06f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = accent;

    // Checkmark, slider
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent_dim;
    colors[ImGuiCol_SliderGrabActive] = accent;

    // Buttons - vibrant on hover
    colors[ImGuiCol_Button] = bg_light;
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive] = accent_dim;

    // Headers (collapsibles, tree nodes)
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = accent_dim;

    // Separators
    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent_hover;

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

    // Tabs - modern flat tabs
    colors[ImGuiCol_Tab] = bg_light;
    colors[ImGuiCol_TabHovered] = accent;
    colors[ImGuiCol_TabActive] = accent_dim;
    colors[ImGuiCol_TabUnfocused] = bg_mid;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.35f, 0.55f, 1.00f);

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_DockingEmptyBg] = bg_dark;

    // Table
    colors[ImGuiCol_TableHeaderBg] = bg_light;
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    // Selection / drag drop
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = accent;

    // Nav highlight
    colors[ImGuiCol_NavHighlight] = accent;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    // Modal dim
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);
}

// Helper to push accent button style (for Play, Stop, etc)
inline void PushAccentButtonStyle(ImVec4 color)
{
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(color.x - 0.1f, color.y - 0.1f, color.z - 0.1f, 1.0f));
}

inline void PopAccentButtonStyle()
{
    ImGui::PopStyleColor(3);
}

} // namespace EditorStyles

#endif // EDITOR_STYLES_H
