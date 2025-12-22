#ifndef EDITOR_STYLES_H
#define EDITOR_STYLES_H

#include <imgui.h>

namespace EditorStyles
{
inline void ApplyDarkTheme()
{
    auto &style = ImGui::GetStyle();
    auto &colors = style.Colors;

    // Visual properties
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.SeparatorTextBorderSize = 1.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(5, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(6, 4);

    // Colors - Unity inspired professional dark theme
    ImVec4 baseColor = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    ImVec4 secondaryBase = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    ImVec4 accentColor = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    ImVec4 activeColor = ImVec4(0.18f, 0.36f, 0.58f, 1.00f); // Professional Blue
    ImVec4 hoverColor = ImVec4(0.24f, 0.44f, 0.68f, 1.00f);

    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = baseColor;
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg] = secondaryBase;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = accentColor;

    colors[ImGuiCol_TitleBg] = secondaryBase;
    colors[ImGuiCol_TitleBgActive] = secondaryBase;
    colors[ImGuiCol_TitleBgCollapsed] = baseColor;

    colors[ImGuiCol_MenuBarBg] = baseColor;
    colors[ImGuiCol_ScrollbarBg] = baseColor;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    colors[ImGuiCol_CheckMark] = activeColor;
    colors[ImGuiCol_SliderGrab] = activeColor;
    colors[ImGuiCol_SliderGrabActive] = hoverColor;

    colors[ImGuiCol_Button] = secondaryBase;
    colors[ImGuiCol_ButtonHovered] = accentColor;
    colors[ImGuiCol_ButtonActive] = activeColor;

    colors[ImGuiCol_Header] = secondaryBase;
    colors[ImGuiCol_HeaderHovered] = accentColor;
    colors[ImGuiCol_HeaderActive] = activeColor;

    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = activeColor;
    colors[ImGuiCol_SeparatorActive] = hoverColor;

    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

    colors[ImGuiCol_Tab] = secondaryBase;
    colors[ImGuiCol_TabHovered] = accentColor;
    colors[ImGuiCol_TabActive] = activeColor;
    colors[ImGuiCol_TabUnfocused] = secondaryBase;
    colors[ImGuiCol_TabUnfocusedActive] = secondaryBase;

    colors[ImGuiCol_DockingPreview] = activeColor;
    colors[ImGuiCol_DockingEmptyBg] = baseColor;
}
} // namespace EditorStyles

#endif // EDITOR_STYLES_H
