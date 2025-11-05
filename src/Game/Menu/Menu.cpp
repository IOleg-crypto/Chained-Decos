#include "Menu.h"
#include "Engine/Collision/CollisionSystem.h"
#include "MenuConstants.h"
#include "SettingsManager.h"
#include "rlImGui.h"
#include <Collision/CollisionStructures.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <sstream>
#include <string>
#include <vector>

Menu::Menu()
    : m_state(MenuState::Main), m_pendingAction(MenuAction::None), m_gameInProgress(false),
      m_selectedMapIndex(0), m_mapsPerPage(MenuConstants::MAPS_PER_PAGE), m_currentPage(0),
      m_totalPages(0), m_jsonMapsCount(0), m_showDemoWindow(false), m_showStyleEditor(false),
      m_settingsManager(std::make_unique<SettingsManager>()), m_consoleManager(nullptr)
{
    // Initialize options vectors
    m_resolutionOptions  = MenuConstants::RESOLUTION_OPTIONS;
    m_displayModeOptions = MenuConstants::DISPLAY_MODE_OPTIONS;
    m_vsyncOptions       = MenuConstants::VSYNC_OPTIONS;
    m_fpsOptions         = MenuConstants::FPS_OPTIONS;
    m_difficultyOptions  = MenuConstants::DIFFICULTY_OPTIONS;

    // Load configuration
    LoadConfiguration();
}

void Menu::Initialize(Engine *engine)
{
    m_engine = engine;
    SetupStyle();
    m_mapSelector = std::make_unique<MapSelector>();
    m_mapSelector->InitializeMaps();
    InitializeMaps();
}

void Menu::SetKernel(Kernel *kernel)
{
    m_kernel = kernel;
    if (m_kernel && !m_consoleManager)
    {
        m_consoleManager = std::make_unique<ConsoleManager>(m_kernel);
    }
}

// IMenuRenderable interface implementations
void Menu::Update()
{
    // Handle keyboard navigation
    HandleKeyboardNavigation();

    // Sync map selection
    if (m_mapSelector)
    {
        m_selectedMapIndex = m_mapSelector->GetSelectedMapIndex();
    }

    // Handle pending actions
    HandlePendingActions();
}

void Menu::Render()
{

    int screenWidth  = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Set up main window for the menu (на весь екран)
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight)),
        ImGuiCond_Always);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Game Menu", nullptr, windowFlags);

    // Render current menu state
    RenderMenuState();

// Debug windows (only in debug builds)
#ifdef _DEBUG
    if (m_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    if (m_showStyleEditor)
    {
        ImGui::Begin("Style Editor", &m_showStyleEditor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
#endif

    ImGui::End();
}

void Menu::BeginFrame() { rlImGuiBegin(); }

void Menu::EndFrame() { rlImGuiEnd(); }

void Menu::SetupStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();

    // Customize colors for a more modern look
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 6.0f;
    style.GrabRounding      = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding       = 8.0f;
    style.ChildRounding     = 8.0f;

    // Improved spacing and sizing
    style.WindowPadding    = ImVec2(16.0f, 16.0f);
    style.FramePadding     = ImVec2(12.0f, 8.0f);
    style.ItemSpacing      = ImVec2(12.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing    = 20.0f;
    style.GrabMinSize      = 20.0f;

    // Modern scrollbar and tab styling
    style.ScrollbarSize = 16.0f;
    style.TabBorderSize = 0.0f;

    // Set up colors with a modern dark theme
    ImVec4 *colors = style.Colors;

    // Window and background colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.98f);
    colors[ImGuiCol_ChildBg]  = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    colors[ImGuiCol_PopupBg]  = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);

    // Title bar colors
    colors[ImGuiCol_TitleBg]          = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

    // Button colors
    colors[ImGuiCol_Button]        = ImVec4(0.25f, 0.25f, 0.25f, 0.8f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.9f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

    // Frame colors
    colors[ImGuiCol_FrameBg]        = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);

    // Slider colors
    colors[ImGuiCol_SliderGrab]       = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);

    // Text colors
    colors[ImGuiCol_Text]         = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    // Border colors
    colors[ImGuiCol_Border]       = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.24f);

    // Scrollbar colors
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    // Tab colors
    colors[ImGuiCol_Tab]        = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_TabActive]  = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

    // Header colors
    colors[ImGuiCol_Header]        = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_HeaderActive]  = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
}

void Menu::RenderMenuState()
{
    switch (m_state)
    {
    case MenuState::Main:
        RenderMainMenu();
        break;
    case MenuState::GameMode:
        RenderGameModeMenu();
        break;
    case MenuState::MapSelection:
        RenderMapSelection();
        break;
    case MenuState::Options:
        RenderOptionsMenu();
        break;
    case MenuState::Video:
        RenderVideoSettings();
        break;
    case MenuState::Audio:
        RenderAudioSettings();
        break;
    case MenuState::Controls:
        RenderControlSettings();
        break;
    case MenuState::Gameplay:
        RenderGameplaySettings();
        break;
    case MenuState::Credits:
        RenderCreditsScreen();
        break;
    case MenuState::Mods:
        RenderModsScreen();
        break;
    case MenuState::ConfirmExit:
        RenderConfirmExitDialog();
        break;
    default:
        RenderMainMenu();
        break;
    }
}

void Menu::RenderMainMenu()
{
    const ImVec2 windowSize  = ImGui::GetWindowSize();
    const float centerX      = windowSize.x * 0.5f;
    const float centerY      = windowSize.y * 0.5f;
    const float buttonWidth  = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing      = 20.0f;

    // Title section
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::PushFont(nullptr); // Use default font but with larger size
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 24.0f);
    ImGui::Text("CHAINED DECOS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::PopStyleColor();

    // Subtitle
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::NAME_FONT_SIZE) / 24.0f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Parkour Adventure");
    ImGui::SetWindowFontScale(1.0f);

    // Menu buttons container
    float startY   = MenuConstants::TOP_MARGIN + 100;
    float currentY = startY;

    if (m_addResumeButton)
    {
        // Start Game button
        ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
        if (RenderActionButton("Resume Game", MenuAction::ResumeGame,
                               ImVec2(buttonWidth, buttonHeight)))
        {
            m_state = MenuState::Resume;
        }
        currentY += buttonHeight + spacing;
    }
    // Start Game button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Start Game", MenuAction::StartGame, ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::GameMode;
    }
    currentY += buttonHeight + spacing;

    // Options button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Options", MenuAction::OpenOptions, ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Options;
    }
    currentY += buttonHeight + spacing;

    // Credits button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Credits", MenuAction::OpenCredits, ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Credits;
    }
    currentY += buttonHeight + spacing;

    // Mods button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Mods", MenuAction::OpenMods, ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Mods;
    }
    currentY += buttonHeight + spacing;

    // Exit Game button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Exit Game", MenuAction::ExitGame, ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::ConfirmExit;
    }

    // Console toggle hint
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, windowSize.y - 40));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::INSTRUCTIONS_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "[~] Console | [F12] Screenshot | [ESC] Back");
    ImGui::SetWindowFontScale(1.0f);
}

void Menu::RenderGameModeMenu()
{
    const ImVec2 windowSize  = ImGui::GetWindowSize();
    const float centerX      = windowSize.x * 0.5f;
    const float centerY      = windowSize.y * 0.5f;
    const float buttonWidth  = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing      = 20.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("GAME MODE SELECTION");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Menu buttons
    float startY   = MenuConstants::TOP_MARGIN + 50;
    float currentY = startY;

    // Single Player button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Single Player", MenuAction::None, ImVec2(buttonWidth, buttonHeight)))
    {
        // Go to map selection for single player
        m_state = MenuState::MapSelection;
    }
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY + 70));
    ImGui::BeginDisabled(true);
    if (RenderActionButton("Multi Player", MenuAction::None, ImVec2(buttonWidth, buttonHeight)))
    {
        // Don`t work - disabled
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(ImVec2(80, windowSize.y - 60));
    RenderBackButton();
}

void Menu::RenderOptionsMenu()
{
    const ImVec2 windowSize  = ImGui::GetWindowSize();
    const float centerX      = windowSize.x * 0.5f;
    const float centerY      = windowSize.y * 0.5f;
    const float buttonWidth  = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing      = 20.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("OPTIONS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Options buttons
    float startY   = MenuConstants::TOP_MARGIN + 50;
    float currentY = startY;

    // Video Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Video Settings", MenuAction::OpenVideoMode,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Video;
    }
    currentY += buttonHeight + spacing;

    // Audio Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Audio Settings", MenuAction::OpenAudio,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Audio;
    }
    currentY += buttonHeight + spacing;

    // Control Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Control Settings", MenuAction::OpenControls,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Controls;
    }
    currentY += buttonHeight + spacing;

    // Gameplay Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Gameplay Settings", MenuAction::OpenGameplay,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Gameplay;
    }

    // Back button
    ImGui::SetCursorPos(ImVec2(80, windowSize.y - 60));
    RenderBackButton();
}

// Helper function to render option combo box with validation
bool Menu::RenderVideoSettingCombo(const char *label, const char *id,
                                   const std::vector<std::string> &options, int &currentIndex,
                                   float labelWidth, float comboWidth, float startX)
{
    // Validate index to prevent out-of-bounds access
    if (currentIndex < 0 || currentIndex >= static_cast<int>(options.size()))
    {
        currentIndex = 0; // Reset to first option if invalid
    }

    const char *currentValue = options[currentIndex].c_str();
    bool changed             = false;

    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", label);
    ImGui::SameLine(startX + labelWidth);
    ImGui::SetNextItemWidth(comboWidth);

    if (ImGui::BeginCombo(id, currentValue))
    {
        for (size_t i = 0; i < options.size(); ++i)
        {
            bool isSelected = (currentIndex == static_cast<int>(i));
            if (ImGui::Selectable(options[i].c_str(), isSelected))
            {
                currentIndex = static_cast<int>(i);
                changed      = true;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}

// Check if video settings have unsaved changes
bool Menu::HasUnsavedVideoChanges() const
{
    if (!m_settingsManager)
        return false;

    return m_videoSettings.resolutionIndex != m_settingsManager->GetResolutionIndex() ||
           m_videoSettings.displayModeIndex != m_settingsManager->GetDisplayModeIndex() ||
           m_videoSettings.vsyncIndex != m_settingsManager->GetVSyncIndex() ||
           m_videoSettings.fpsIndex != m_settingsManager->GetFpsIndex();
}

void Menu::RenderVideoSettings()
{
    const ImVec2 windowSize   = ImGui::GetWindowSize();
    const float labelWidth    = 200.0f;
    const float comboWidth    = 200.0f;
    const float startX        = MenuConstants::MARGIN + 50.0f;
    const float startY        = MenuConstants::MARGIN + 60.0f;
    const float spacing       = 50.0f;
    const float buttonSpacing = 140.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(startX, MenuConstants::MARGIN + 20));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("VIDEO SETTINGS");
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);

    // Check for unsaved changes indicator
    bool hasUnsavedChanges = HasUnsavedVideoChanges();
    if (hasUnsavedChanges)
    {
        ImGui::SetCursorPos(ImVec2(startX, MenuConstants::MARGIN + 50));
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "* Unsaved changes");
    }

    ImGui::SetCursorPos(ImVec2(startX, startY));

    // Resolution
    RenderVideoSettingCombo("Resolution", "##resolution", m_resolutionOptions,
                            m_videoSettings.resolutionIndex, labelWidth, comboWidth, startX);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);

    // Display Mode
    RenderVideoSettingCombo("Display Mode", "##display_mode", m_displayModeOptions,
                            m_videoSettings.displayModeIndex, labelWidth, comboWidth, startX);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);

    // VSync
    RenderVideoSettingCombo("VSync", "##vsync", m_vsyncOptions, m_videoSettings.vsyncIndex,
                            labelWidth, comboWidth, startX);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);

    // FPS Limit
    RenderVideoSettingCombo("FPS Limit", "##fps", m_fpsOptions, m_videoSettings.fpsIndex,
                            labelWidth, comboWidth, startX);

    // Buttons section
    ImGui::SetCursorPos(ImVec2(startX, windowSize.y - 80));

    // Apply button - enable only if there are unsaved changes
    ImGui::BeginDisabled(!hasUnsavedChanges);
    if (ImGui::Button("Apply", ImVec2(120, 40)) || (hasUnsavedChanges && IsKeyPressed(KEY_ENTER)))
    {
        if (m_settingsManager)
        {
            // Validate all indices before applying
            if (m_videoSettings.resolutionIndex >= 0 &&
                m_videoSettings.resolutionIndex < static_cast<int>(m_resolutionOptions.size()) &&
                m_videoSettings.displayModeIndex >= 0 &&
                m_videoSettings.displayModeIndex < static_cast<int>(m_displayModeOptions.size()) &&
                m_videoSettings.vsyncIndex >= 0 &&
                m_videoSettings.vsyncIndex < static_cast<int>(m_vsyncOptions.size()) &&
                m_videoSettings.fpsIndex >= 0 &&
                m_videoSettings.fpsIndex < static_cast<int>(m_fpsOptions.size()))
            {
                m_settingsManager->SetResolutionIndex(m_videoSettings.resolutionIndex);
                m_settingsManager->SetDisplayModeIndex(m_videoSettings.displayModeIndex);
                m_settingsManager->SetVSyncIndex(m_videoSettings.vsyncIndex);
                m_settingsManager->SetFpsIndex(m_videoSettings.fpsIndex);
                m_settingsManager->ApplyVideoSettings();
                m_settingsManager->SaveSettings();
            }
        }
        m_pendingAction = MenuAction::ApplyVideoSettings;
    }
    ImGui::EndDisabled();

    ImGui::SameLine(startX + buttonSpacing);
    RenderBackButton();
}

void Menu::RenderAudioSettings()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::MARGIN + 20));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::NAME_FONT_SIZE) / 24.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "AUDIO SETTINGS");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Dummy(ImVec2(0, 40));

    // Master Volume Slider
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN + 30));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Master Volume");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(250);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
    ImGui::SliderFloat("##master_vol", &m_audioSettings.masterVolume, 0.0f, 1.0f, "%.0f%%");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));

    // Music Volume Slider
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Music Volume");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
    ImGui::SliderFloat("##music_vol", &m_audioSettings.musicVolume, 0.0f, 1.0f, "%.0f%%");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));

    // SFX Volume Slider
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "SFX Volume");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
    ImGui::SliderFloat("##sfx_vol", &m_audioSettings.sfxVolume, 0.0f, 1.0f, "%.0f%%");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));

    // Mute Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Mute Audio");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(250);
    ImGui::Checkbox("##mute", &m_audioSettings.muted);

    // Apply and Back buttons
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, windowSize.y - 80));
    if (ImGui::Button("Apply", ImVec2(120, 40)) || IsKeyPressed(KEY_ENTER))
    {
        m_pendingAction = MenuAction::ApplyAudioSettings;
    }

    ImGui::SameLine();
    RenderBackButton();
}

void Menu::RenderControlSettings()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::MARGIN + 20));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::NAME_FONT_SIZE) / 24.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "CONTROL SETTINGS");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Dummy(ImVec2(0, 40));

    // Mouse Sensitivity Slider
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN + 30));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Mouse Sensitivity");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SliderFloat("##mouse_sens", &m_controlSettings.mouseSensitivity, 0.1f, 3.0f, "%.1fx");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));

    // Invert Y Axis Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Invert Y Axis");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::Checkbox("##invert_y", &m_controlSettings.invertYAxis);

    ImGui::Dummy(ImVec2(0, 20));

    // Controller Support Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Controller Support");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(250);
    ImGui::Checkbox("##controller", &m_controlSettings.controllerSupport);

    // Apply and Back buttons
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, windowSize.y - 80));
    if (ImGui::Button("Apply", ImVec2(120, 40)) || IsKeyPressed(KEY_ENTER))
    {
        m_pendingAction = MenuAction::ApplyControlSettings;
    }

    ImGui::SameLine();
    RenderBackButton();
}

void Menu::RenderGameplaySettings()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::MARGIN + 20));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::NAME_FONT_SIZE) / 24.0f);
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "GAMEPLAY SETTINGS");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Dummy(ImVec2(0, 40));

    // Difficulty setting
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN + 30));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Difficulty");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    if (ImGui::BeginCombo("##difficulty",
                          m_difficultyOptions[m_gameplaySettings.difficultyLevel - 1].c_str()))
    {
        for (size_t i = 0; i < m_difficultyOptions.size(); ++i)
        {
            bool isSelected = (m_gameplaySettings.difficultyLevel - 1 == static_cast<int>(i));
            if (ImGui::Selectable(m_difficultyOptions[i].c_str(), isSelected))
            {
                m_gameplaySettings.difficultyLevel = static_cast<int>(i) + 1;
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Dummy(ImVec2(0, 20));

    // Timer Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Timer");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::Checkbox("##timer", &m_gameplaySettings.timerEnabled);

    ImGui::Dummy(ImVec2(0, 20));

    // Checkpoints Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Checkpoints");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::Checkbox("##checkpoints", &m_gameplaySettings.checkpointsEnabled);

    ImGui::Dummy(ImVec2(0, 20));

    // Auto Save Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Auto Save");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(280);
    ImGui::Checkbox("##auto_save", &m_gameplaySettings.autoSaveEnabled);

    ImGui::Dummy(ImVec2(0, 20));

    // Speedrun Mode Toggle
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::DESCRIPTION_FONT_SIZE) / 16.0f);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Speedrun Mode");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(250);
    ImGui::Checkbox("##speedrun", &m_gameplaySettings.speedrunMode);

    // Apply and Back buttons
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, windowSize.y - 80));
    if (ImGui::Button("Apply", ImVec2(120, 40)) || IsKeyPressed(KEY_ENTER))
    {
        m_pendingAction = MenuAction::ApplyGameplaySettings;
    }

    ImGui::SameLine();
    RenderBackButton();
}

void Menu::RenderMapSelection()
{
    if (m_mapSelector)
    {
        m_mapSelector->RenderMapSelectionWindow();

        // Sync selection after rendering
        m_selectedMapIndex = m_mapSelector->GetSelectedMapIndex();

        // Start Game button
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const float centerX     = windowSize.x * 0.5f;
        ImGui::SetCursorPos(ImVec2(centerX - 160, windowSize.y - 100));
        RenderActionButton("Start Game with Selected Map", MenuAction::StartGameWithMap,
                           ImVec2(320, 50));
    }
    else
    {
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const float centerX     = windowSize.x * 0.5f;
        ImGui::SetCursorPos(ImVec2(centerX - 100, 150));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No maps available");
    }

    // Back button
    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2(80, windowSize.y - 60));
    RenderBackButton();
}

void Menu::RenderCreditsScreen()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX     = windowSize.x * 0.5f;
    const float centerY     = windowSize.y * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 28.0f);
    ImGui::Text("CREDITS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Credits content
    float startY         = centerY - 120;
    float currentY       = startY;
    float sectionSpacing = 60.0f;
    float labelSpacing   = 30.0f;

    // Developer section
    ImGui::SetCursorPos(ImVec2(centerX - 100, currentY));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "DEVELOPER");
    currentY += labelSpacing;
    ImGui::SetCursorPos(ImVec2(centerX - 50, currentY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "I#Oleg");
    currentY += sectionSpacing;

    // Engine section
    ImGui::SetCursorPos(ImVec2(centerX - 100, currentY));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "ENGINE");
    currentY += labelSpacing;
    ImGui::SetCursorPos(ImVec2(centerX - 80, currentY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "raylib + rlImGui");
    currentY += sectionSpacing;

    // UI Design section
    ImGui::SetCursorPos(ImVec2(centerX - 100, currentY));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "UI DESIGN");
    currentY += labelSpacing;
    ImGui::SetCursorPos(ImVec2(centerX - 80, currentY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "Modern Interface");

    // Back button
    ImGui::SetCursorPos(ImVec2(centerX - 40, windowSize.y - 60));
    RenderBackButton();
}

void Menu::RenderModsScreen()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX     = windowSize.x * 0.5f;
    const float centerY     = windowSize.y * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 28.0f);
    ImGui::Text("MODS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Content
    ImGui::SetCursorPos(ImVec2(centerX - 120, centerY - 100));
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "NO MODS DETECTED");

    ImGui::SetCursorPos(ImVec2(centerX - 220, centerY - 60));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f),
                       "Place your mods in the 'resources/mods' folder");

    // Back button
    ImGui::SetCursorPos(ImVec2(centerX - 40, windowSize.y - 60));
    RenderBackButton();
}

void Menu::RenderConfirmExitDialog()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();

    // Semi-transparent background
    ImGui::GetForegroundDrawList()->AddRectFilled(
        ImVec2(0, 0), ImVec2(windowSize.x, windowSize.y),
        ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.7f)));

    // Modal window
    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2 - 200, windowSize.y / 2 - 150));
    ImGui::SetNextWindowSize(ImVec2(400, 300));

    ImGui::Begin("Exit Confirmation", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Title
    ImGui::SetCursorPos(ImVec2(150, 40));
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "EXIT GAME?");

    // Buttons
    ImGui::SetCursorPos(ImVec2(80, 200));

    if (ImGui::Button("YES", ImVec2(80, 40)))
    {
        m_pendingAction = MenuAction::ExitGame;
    }

    ImGui::SameLine();

    ImGui::SetCursorPos(ImVec2(240, 200));

    if (ImGui::Button("NO", ImVec2(80, 40)))
    {
        m_state = MenuState::Main;
    }

    // Instructions
    ImGui::SetCursorPos(ImVec2(120, 260));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "Y/ENTER = Yes    N/ESC = No");

    ImGui::End();
}

// Console functionality removed as it's now handled directly in Render

// Helper methods
void Menu::HandlePendingActions()
{
    if (m_pendingAction != MenuAction::None)
    {
        switch (m_pendingAction)
        {
        case MenuAction::ApplyVideoSettings:
            SyncVideoSettingsToConfig();
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::ApplyAudioSettings:
            SyncAudioSettingsToConfig();
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::ApplyControlSettings:
            SyncControlSettingsToConfig();
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::ApplyGameplaySettings:
            SyncGameplaySettingsToConfig();
            m_pendingAction = MenuAction::None;
            break;
        default:
            // Don't reset m_pendingAction for actions handled by MenuActionHandler
            // (SinglePlayer, ResumeGame, StartGameWithMap, ExitGame, etc.)
            break;
        }
    }
}

void Menu::HandleKeyboardNavigation()
{
    // Handle ESC key to go back
    if (IsKeyPressed(KEY_ESCAPE))
    {
        switch (m_state)
        {
        case MenuState::GameMode:
        case MenuState::MapSelection:
        case MenuState::Options:
        case MenuState::Video:
        case MenuState::Audio:
        case MenuState::Controls:
        case MenuState::Gameplay:
        case MenuState::Credits:
        case MenuState::Mods:
            m_state = MenuState::Main;
            break;
        case MenuState::ConfirmExit:
            m_state = MenuState::Main;
            break;
        default:
            break;
        }
    }

    // Handle console toggle (tilde key)
    if (IsKeyPressed(KEY_GRAVE))
    {
        TraceLog(LOG_INFO, "Menu::HandleKeyboardNavigation() - Console toggle key pressed");
        ToggleConsole();
    }

    // Handle specific navigation for map selection
    if (m_state == MenuState::MapSelection && m_mapSelector && m_mapSelector->HasMaps())
    {
        m_mapSelector->HandleKeyboardNavigation();
        if (IsKeyPressed(KEY_ENTER))
        {
            m_pendingAction = MenuAction::StartGameWithMap;
        }
        // Sync selection
        m_selectedMapIndex = m_mapSelector->GetSelectedMapIndex();
    }

    // Handle navigation for other menus with arrow keys
    if (m_state == MenuState::Main || m_state == MenuState::GameMode ||
        m_state == MenuState::Options)
    {
        if (IsKeyPressed(KEY_UP))
        {
            // Navigate up in menu (could be implemented with a selected item index)
        }
        else if (IsKeyPressed(KEY_DOWN))
        {
            // Navigate down in menu
        }
        else if (IsKeyPressed(KEY_ENTER))
        {
            // Activate selected item (could be implemented)
        }
    }
}

bool Menu::RenderActionButton(const char *label, MenuAction action, const ImVec2 &size)
{
    // Enhanced button styling
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleColor(4);

    if (clicked && action != MenuAction::None)
    {
        TraceLog(LOG_INFO, "Menu::RenderActionButton() - Button '%s' clicked, setting action: %d",
                 label, static_cast<int>(action));
        m_pendingAction = action;
        return true;
    }
    return clicked;
}

bool Menu::RenderBackButton(float width)
{
    ImVec2 buttonSize(width > 0 ? width : 100, 30);

    // Enhanced back button styling
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

    bool clicked = ImGui::Button("Back", buttonSize);

    ImGui::PopStyleColor(4);

    if (clicked)
    {
        m_state = MenuState::Main;
        return true;
    }
    return false;
}

void Menu::RenderSectionHeader(const char *title, const char *subtitle) const
{
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), title);
    if (subtitle)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), subtitle);
    }
}

void Menu::RenderMenuHint(const char *text) const
{
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), text);
}

void Menu::RenderMapCard(int /*index*/, const MapInfo &map, bool selected, float cardWidth) const
{
    if (selected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 1.0f, 0.8f));
    }

    ImGui::Button(map.displayName.c_str(), ImVec2(cardWidth, 40));

    if (selected)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), map.description.c_str());
}

// Pagination methods
void Menu::EnsurePagination()
{
    if (m_availableMaps.empty())
    {
        m_currentPage = 0;
        return;
    }

    int totalPages = (static_cast<int>(m_availableMaps.size()) + m_mapsPerPage - 1) / m_mapsPerPage;
    if (m_currentPage >= totalPages)
        m_currentPage = std::max(0, totalPages - 1);
}

void Menu::GoToNextPage()
{
    if (m_currentPage < GetTotalPages() - 1)
        m_currentPage++;
}

void Menu::GoToPreviousPage()
{
    if (m_currentPage > 0)
        m_currentPage--;
}

int Menu::GetPageStartIndex() const { return m_currentPage * m_mapsPerPage; }

int Menu::GetPageEndIndex() const
{
    return std::min(GetPageStartIndex() + m_mapsPerPage, static_cast<int>(m_availableMaps.size()));
}

int Menu::GetTotalPages() const
{
    if (m_availableMaps.empty())
        return 0;
    return (static_cast<int>(m_availableMaps.size()) + m_mapsPerPage - 1) / m_mapsPerPage;
}

void Menu::RenderPaginationControls()
{
    int totalPages = GetTotalPages();
    if (totalPages <= 1)
        return;

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Text("Page %d of %d", m_currentPage + 1, totalPages);

    if (m_currentPage > 0 && ImGui::Button("Previous Page"))
    {
        GoToPreviousPage();
    }

    ImGui::SameLine();

    if (m_currentPage < totalPages - 1 && ImGui::Button("Next Page"))
    {
        GoToNextPage();
    }
}

// Settings synchronization methods
void Menu::SyncVideoSettingsToConfig()
{
    if (m_settingsManager)
    {
        m_settingsManager->SetResolutionIndex(m_videoSettings.resolutionIndex);
        m_settingsManager->SetDisplayModeIndex(m_videoSettings.displayModeIndex);
        m_settingsManager->SetVSyncIndex(m_videoSettings.vsyncIndex);
        m_settingsManager->SetFpsIndex(m_videoSettings.fpsIndex);
        m_settingsManager->ApplyVideoSettings();
        m_settingsManager->SaveSettings(); // Зберігаємо налаштування в конфіг
    }
}

void Menu::SyncAudioSettingsToConfig()
{
    if (m_settingsManager)
    {
        m_settingsManager->SetMasterVolume(m_audioSettings.masterVolume);
        m_settingsManager->SetMusicVolume(m_audioSettings.musicVolume);
        m_settingsManager->SetSfxVolume(m_audioSettings.sfxVolume);
        m_settingsManager->SetMuted(m_audioSettings.muted);
        m_settingsManager->ApplyAudioSettings();
        m_settingsManager->SaveSettings(); // Зберігаємо налаштування в конфіг
    }
}

void Menu::SyncControlSettingsToConfig()
{
    if (m_settingsManager)
    {
        m_settingsManager->SetMouseSensitivity(m_controlSettings.mouseSensitivity);
        m_settingsManager->SetInvertYAxis(m_controlSettings.invertYAxis);
        m_settingsManager->SetControllerSupport(m_controlSettings.controllerSupport);
        m_settingsManager->SaveSettings(); // Зберігаємо налаштування в конфіг
    }
}

void Menu::SyncGameplaySettingsToConfig()
{
    if (m_settingsManager)
    {
        m_settingsManager->SetDifficultyLevel(m_gameplaySettings.difficultyLevel);
        m_settingsManager->SetTimerEnabled(m_gameplaySettings.timerEnabled);
        m_settingsManager->SetCheckpointsEnabled(m_gameplaySettings.checkpointsEnabled);
        m_settingsManager->SetAutoSaveEnabled(m_gameplaySettings.autoSaveEnabled);
        m_settingsManager->SetSpeedrunMode(m_gameplaySettings.speedrunMode);
        m_settingsManager->SaveSettings(); // Зберігаємо налаштування в конфіг
    }
}

// State management
void Menu::SetGameInProgress(bool inProgress) { m_gameInProgress = inProgress; }

bool Menu::IsGameInProgress() const { return m_gameInProgress; }

MenuAction Menu::ConsumeAction()
{
    MenuAction action = m_pendingAction;
    m_pendingAction   = MenuAction::None;
    return action;
}

MenuState Menu::GetState() const { return m_state; }

void Menu::SetState(MenuState state) { m_state = state; }

// Navigation methods
void Menu::ShowMainMenu() { m_state = MenuState::Main; }
void Menu::ShowOptionsMenu() { m_state = MenuState::Options; }
void Menu::ShowGameModeMenu() { m_state = MenuState::GameMode; }
void Menu::ShowMapSelection() { m_state = MenuState::MapSelection; }
void Menu::ShowAudioMenu() { m_state = MenuState::Audio; }
void Menu::ShowVideoMenu() { m_state = MenuState::Video; }
void Menu::ShowControlsMenu() { m_state = MenuState::Controls; }
void Menu::ShowGameplayMenu() { m_state = MenuState::Gameplay; }
void Menu::ShowCredits() { m_state = MenuState::Credits; }
void Menu::ShowMods() { m_state = MenuState::Mods; }
void Menu::ShowConfirmExit() { m_state = MenuState::ConfirmExit; }

// Apply pending settings
void Menu::ApplyPendingSettings()
{
    // Apply video settings
    SyncVideoSettingsToConfig();
    // Apply audio settings
    SyncAudioSettingsToConfig();
    // Apply control settings
    SyncControlSettingsToConfig();
    // Apply gameplay settings
    SyncGameplaySettingsToConfig();

    // Save configuration
    SaveConfiguration();
}

// Get selected map
std::optional<MapInfo> Menu::GetSelectedMap() const
{
    if (m_mapSelector)
    {
        const MapInfo *selected = m_mapSelector->GetSelectedMap();
        if (selected)
        {
            return *selected;
        }
    }
    return std::nullopt;
}

std::string Menu::GetSelectedMapName() const
{
    if (m_mapSelector)
    {
        return m_mapSelector->GetSelectedMapName();
    }
    return "";
}

// Initialize maps
void Menu::InitializeMaps()
{
    // Sync with MapSelector
    m_availableMaps    = m_mapSelector->GetAvailableMaps();
    m_selectedMapIndex = m_mapSelector->GetSelectedMapIndex();
    m_currentPage      = m_mapSelector->GetCurrentPage();
    m_totalPages       = m_mapSelector->GetTotalPages();
    m_jsonMapsCount    = m_mapSelector->GetJsonMapsCount();
}

void Menu::ScanForJsonMaps()
{
    m_jsonMapsCount = 0;

    // Scan for JSON maps in the resources/maps directory
    std::string mapsPath = PROJECT_ROOT_DIR "resources/maps";

    try
    {
        if (std::filesystem::exists(mapsPath) && std::filesystem::is_directory(mapsPath))
        {
            for (const auto &entry : std::filesystem::directory_iterator(mapsPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    std::string filename = entry.path().stem().string();
                    std::string filepath = entry.path().string();

                    // Create MapInfo for this JSON file
                    MapInfo jsonMap;
                    jsonMap.name = filename;

                    // Format display name from filename
                    std::string displayName = filename;
                    std::replace(displayName.begin(), displayName.end(), '_', ' ');
                    bool capitalize = true;
                    for (char &c : displayName)
                    {
                        if (capitalize && std::isalpha(c))
                        {
                            c          = std::toupper(c);
                            capitalize = false;
                        }
                        else if (std::isspace(c))
                        {
                            capitalize = true;
                        }
                        else
                        {
                            capitalize = false;
                        }
                    }
                    jsonMap.displayName = displayName;

                    // Generate description by analyzing the JSON file
                    std::ifstream mapFile(filepath);
                    std::string fileContent((std::istreambuf_iterator<char>(mapFile)),
                                            std::istreambuf_iterator<char>());
                    size_t objectCount = 0;
                    size_t pos         = 0;
                    while ((pos = fileContent.find("{", pos)) != std::string::npos)
                    {
                        objectCount++;
                        pos++;
                    }
                    jsonMap.description  = "Map with " + std::to_string(objectCount) + " objects";
                    jsonMap.previewImage = "";
                    jsonMap.themeColor   = SKYBLUE;
                    jsonMap.isAvailable  = true;
                    jsonMap.isModelBased = false;

                    m_availableMaps.push_back(jsonMap);
                    m_jsonMapsCount++;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error scanning for JSON maps: " << e.what() << std::endl;
    }
}

void Menu::UpdatePagination() { m_totalPages = GetTotalPages(); }

// Save configuration
void Menu::SaveConfiguration()
{
    if (m_settingsManager)
    {
        m_settingsManager->SaveSettings();
    }
}

// Load configuration
void Menu::LoadConfiguration()
{
    if (m_settingsManager)
    {
        m_settingsManager->LoadSettings();

        // Load settings into local variables
        m_audioSettings.masterVolume = m_settingsManager->GetMasterVolume();
        m_audioSettings.musicVolume  = m_settingsManager->GetMusicVolume();
        m_audioSettings.sfxVolume    = m_settingsManager->GetSfxVolume();
        m_audioSettings.muted        = m_settingsManager->IsMuted();

        m_controlSettings.mouseSensitivity  = m_settingsManager->GetMouseSensitivity();
        m_controlSettings.invertYAxis       = m_settingsManager->GetInvertYAxis();
        m_controlSettings.controllerSupport = m_settingsManager->GetControllerSupport();

        m_gameplaySettings.difficultyLevel    = m_settingsManager->GetDifficultyLevel();
        m_gameplaySettings.timerEnabled       = m_settingsManager->IsTimerEnabled();
        m_gameplaySettings.checkpointsEnabled = m_settingsManager->AreCheckpointsEnabled();
        m_gameplaySettings.autoSaveEnabled    = m_settingsManager->IsAutoSaveEnabled();
        m_gameplaySettings.speedrunMode       = m_settingsManager->IsSpeedrunMode();

        m_videoSettings.resolutionIndex  = m_settingsManager->GetResolutionIndex();
        m_videoSettings.displayModeIndex = m_settingsManager->GetDisplayModeIndex();
        m_videoSettings.vsyncIndex       = m_settingsManager->GetVSyncIndex();
        m_videoSettings.fpsIndex         = m_settingsManager->GetFpsIndex();
    }
}

// Action management
void Menu::SetAction(MenuAction action) { m_pendingAction = action; }

MenuAction Menu::GetAction() const { return m_pendingAction; }

void Menu::ResetAction() { m_pendingAction = MenuAction::None; }

// Console functionality
void Menu::ToggleConsole()
{
    if (m_consoleManager)
    {
        m_consoleManager->ToggleConsole();
    }
}

bool Menu::IsConsoleOpen() const { return m_consoleManager && m_consoleManager->IsConsoleOpen(); }

// Helper methods
void Menu::HandleAction(MenuAction action) { m_pendingAction = action; }

const char *Menu::GetStateTitle(MenuState state)
{
    switch (state)
    {
    case MenuState::Main:
        return "CHAINED DECOS";
    case MenuState::Options:
        return "OPTIONS";
    case MenuState::Video:
        return "VIDEO SETTINGS";
    case MenuState::Audio:
        return "AUDIO SETTINGS";
    case MenuState::Controls:
        return "CONTROL SETTINGS";
    case MenuState::Gameplay:
        return "GAMEPLAY SETTINGS";
    case MenuState::GameMode:
        return "GAME MODE";
    case MenuState::MapSelection:
        return "MAP SELECTION";
    case MenuState::Credits:
        return "CREDITS";
    case MenuState::Mods:
        return "MODS";
    case MenuState::ConfirmExit:
        return "EXIT GAME?";
    default:
        return "MENU";
    }
}

void Menu::SetResumeButtonOn(bool status) { m_addResumeButton = status; }

bool Menu::GetResumeButtonStatus() const { return m_addResumeButton; }

ConsoleManager *Menu::GetConsoleManager() const { return m_consoleManager.get(); }
