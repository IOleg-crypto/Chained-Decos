#include "MenuController.h"
#include "Engine/Engine.h"
#include <raylib.h>
#include <iostream>
#include <imgui.h>
#include <rlImGui/rlImGui.h>

// Unified ImGui-based menu system implementation
// Provides clean separation between menu logic and rendering

void MenuController::Initialize(Menu* menu)
{
    if (menu)
    {
        menu->Initialize(nullptr); // Engine will be set later
        SetupImGuiStyle();
    }
}

void MenuController::Update(Menu* menu)
{
    if (menu)
    {
        menu->Update();
    }
}

void MenuController::Render(Menu* menu)
{
    if (menu)
    {
        menu->Render();
    }
}

void MenuController::ExecuteAction(Menu* menu)
{
    if (menu)
    {
        MenuAction action = menu->ConsumeAction();
        if (action != MenuAction::None)
        {
            // Handle the action here if needed
            // For now, the Menu class handles its own actions
        }
    }
}

MenuAction MenuController::GetAction(Menu* menu)
{
    if (menu)
    {
        return menu->ConsumeAction();
    }
    return MenuAction::None;
}

void MenuController::SetAction(Menu* menu, MenuAction action)
{
    if (menu)
    {
        menu->SetAction(action);
    }
}

void MenuController::LoadSettings(Menu* menu)
{
    if (menu)
    {
        menu->LoadConfiguration();
    }
}

void MenuController::SaveSettings(Menu* menu)
{
    if (menu)
    {
        menu->SaveConfiguration();
    }
}

void MenuController::SetGameInProgress(Menu* menu, bool inProgress)
{
    if (menu)
    {
        menu->SetGameInProgress(inProgress);
    }
}

// Enhanced theme system with modern color scheme
void MenuController::SetupImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Modern rounded corners and spacing
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 8.0f;

    // Improved spacing and sizing
    style.WindowPadding = ImVec2(16.0f, 16.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(12.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 20.0f;

    // Modern scrollbar and tab styling
    style.ScrollbarSize = 16.0f;
    style.TabBorderSize = 0.0f;

    // Setup modern dark theme colors
    SetupModernDarkTheme();
}

// Helper function to mix two colors
ImVec4 MixColors(const ImVec4& a, const ImVec4& b, float ratio)
{
    return {
        a.x * (1.0f - ratio) + b.x * ratio,
        a.y * (1.0f - ratio) + b.y * ratio,
        a.z * (1.0f - ratio) + b.z * ratio,
        a.w * (1.0f - ratio) + b.w * ratio
    };
}

// Modern dark theme with enhanced contrast and readability
void MenuController::SetupModernDarkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Modern dark color palette
    const ImVec4 colorBackground = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    const ImVec4 colorSurface = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    const ImVec4 colorSurfaceVariant = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    const ImVec4 colorPrimary = ImVec4(0.25f, 0.45f, 0.85f, 1.00f);
    const ImVec4 colorPrimaryVariant = ImVec4(0.35f, 0.55f, 0.95f, 1.00f);
    const ImVec4 colorSecondary = ImVec4(0.45f, 0.25f, 0.65f, 1.00f);
    const ImVec4 colorAccent = ImVec4(0.85f, 0.45f, 0.25f, 1.00f);
    const ImVec4 colorText = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    const ImVec4 colorTextSecondary = ImVec4(0.75f, 0.75f, 0.78f, 1.00f);
    const ImVec4 colorBorder = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);

    // Window colors
    colors[ImGuiCol_WindowBg] = colorBackground;
    colors[ImGuiCol_ChildBg] = colorSurface;
    colors[ImGuiCol_PopupBg] = colorSurface;

    // Title colors
    colors[ImGuiCol_TitleBg] = colorSurface;
    colors[ImGuiCol_TitleBgActive] = colorSurfaceVariant;
    colors[ImGuiCol_TitleBgCollapsed] = colorSurface;

    // Menu colors
    colors[ImGuiCol_MenuBarBg] = colorSurface;

    // Frame colors (input fields, checkboxes, etc.)
    colors[ImGuiCol_FrameBg] = colorSurfaceVariant;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(colorSurfaceVariant.x + 0.05f, colorSurfaceVariant.y + 0.05f, colorSurfaceVariant.z + 0.05f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = MixColors(colorPrimary, colorSurfaceVariant, 0.4f);

    // Button colors
    colors[ImGuiCol_Button] = colorSurfaceVariant;
    colors[ImGuiCol_ButtonHovered] = MixColors(colorPrimary, colorSurfaceVariant, 0.6f);
    colors[ImGuiCol_ButtonActive] = MixColors(colorPrimary, colorSurfaceVariant, 0.8f);

    // Header colors
    colors[ImGuiCol_Header] = MixColors(colorPrimary, colorSurface, 0.3f);
    colors[ImGuiCol_HeaderHovered] = MixColors(colorPrimary, colorSurface, 0.5f);
    colors[ImGuiCol_HeaderActive] = colorPrimary;

    // Tab colors
    colors[ImGuiCol_Tab] = colorSurface;
    colors[ImGuiCol_TabHovered] = colorSurfaceVariant;
    colors[ImGuiCol_TabActive] = MixColors(colorPrimary, colorSurface, 0.3f);
    colors[ImGuiCol_TabUnfocused] = colorSurface;
    colors[ImGuiCol_TabUnfocusedActive] = colorSurfaceVariant;

    // Text colors
    colors[ImGuiCol_Text] = colorText;
    colors[ImGuiCol_TextDisabled] = ImVec4(colorTextSecondary.x * 0.6f, colorTextSecondary.y * 0.6f, colorTextSecondary.z * 0.6f, 1.0f);

    // Border and separator colors
    colors[ImGuiCol_Border] = colorBorder;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_Separator] = colorBorder;
    colors[ImGuiCol_SeparatorHovered] = MixColors(colorPrimary, colorBorder, 0.5f);
    colors[ImGuiCol_SeparatorActive] = colorPrimary;

    // Scrollbar colors
    colors[ImGuiCol_ScrollbarBg] = colorBackground;
    colors[ImGuiCol_ScrollbarGrab] = colorSurfaceVariant;
    colors[ImGuiCol_ScrollbarGrabHovered] = MixColors(colorPrimary, colorSurfaceVariant, 0.4f);
    colors[ImGuiCol_ScrollbarGrabActive] = MixColors(colorPrimary, colorSurfaceVariant, 0.6f);

    // Slider colors
    colors[ImGuiCol_SliderGrab] = colorPrimary;
    colors[ImGuiCol_SliderGrabActive] = colorPrimaryVariant;

    // Check mark color
    colors[ImGuiCol_CheckMark] = colorPrimary;

    // Resize grip colors
    colors[ImGuiCol_ResizeGrip] = colorSurfaceVariant;
    colors[ImGuiCol_ResizeGripHovered] = MixColors(colorPrimary, colorSurfaceVariant, 0.4f);
    colors[ImGuiCol_ResizeGripActive] = MixColors(colorPrimary, colorSurfaceVariant, 0.6f);

    // Plot colors
    colors[ImGuiCol_PlotLines] = colorPrimary;
    colors[ImGuiCol_PlotLinesHovered] = colorPrimaryVariant;
    colors[ImGuiCol_PlotHistogram] = colorPrimary;
    colors[ImGuiCol_PlotHistogramHovered] = colorPrimaryVariant;

    // Modal window colors
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);

    // Navigation colors
    colors[ImGuiCol_NavHighlight] = colorPrimary;
    colors[ImGuiCol_NavWindowingHighlight] = colorAccent;
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

    // Selection colors
    colors[ImGuiCol_DragDropTarget] = colorAccent;
    colors[ImGuiCol_TableHeaderBg] = colorSurface;
    colors[ImGuiCol_TableBorderStrong] = colorBorder;
    colors[ImGuiCol_TableBorderLight] = ImVec4(colorBorder.x * 0.7f, colorBorder.y * 0.7f, colorBorder.z * 0.7f, 1.0f);
    colors[ImGuiCol_TableRowBg] = colorBackground;
    colors[ImGuiCol_TableRowBgAlt] = MixColors(colorSurface, colorBackground, 0.3f);
}

void MenuController::BeginImGuiFrame()
{
    rlImGuiBegin();
}

void MenuController::EndImGuiFrame()
{
    rlImGuiEnd();
}

void MenuController::ApplyCustomTheme(Menu* menu)
{
    if (menu)
    {
        // Apply any custom theme settings from the menu
        // This could be used for user-customizable themes
    }
}

void MenuController::ResetToDefaultTheme()
{
    SetupModernDarkTheme();
}

bool MenuController::IsMenuVisible(Menu* menu)
{
    if (menu)
    {
        return menu->GetState() != MenuState::Main || menu->IsConsoleOpen();
    }
    return false;
}

void MenuController::ToggleMenuVisibility(Menu* menu)
{
    if (menu)
    {
        if (menu->IsConsoleOpen())
        {
            menu->ToggleConsole();
        }
        else
        {
            menu->ShowMainMenu();
        }
    }
}
