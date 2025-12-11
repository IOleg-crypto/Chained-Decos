#include "Menu.h"
#include "MenuConstants.h"
#include "Settings/SettingsManager.h"

#include "components/physics/collision/System/CollisionSystem.h"
#include "core/engine/Engine.h"
#include "rlImGui.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <components/physics/collision/Structures/CollisionStructures.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

Menu::Menu()
    : m_state(MenuState::Main), m_pendingAction(MenuAction::None), m_gameInProgress(false),
      m_showDemoWindow(false), m_showStyleEditor(false),
      m_settingsManager(std::make_unique<SettingsManager>()),
      m_consoleManager(std::make_unique<ConsoleManager>()),
      m_mapSelector(std::make_unique<MapSelector>()), m_presenter(std::make_unique<MenuPresenter>())
{
    // Collect all resolutions in a set to automatically remove duplicates
    std::set<std::string> resolutionSet;

    // Get available monitor resolutions using GLFW
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (monitor != nullptr)
    {
        int modesCount = 0;
        const GLFWvidmode *modes = glfwGetVideoModes(monitor, &modesCount);

        if (modes != nullptr && modesCount > 0)
        {
            for (int i = 0; i < modesCount; ++i)
            {
                resolutionSet.insert(std::to_string(modes[i].width) + "x" +
                                     std::to_string(modes[i].height));
            }
        }
    }

    // Add standard resolution options (for windowed mode and fallback)
    for (const auto &resolution : MenuConstants::RESOLUTION_OPTIONS)
    {
        if (!resolution.empty())
        {
            resolutionSet.insert(resolution);
        }
    }

    // Initialize MenuSettingsController
    m_settingsController = std::make_unique<MenuSettingsController>();
    m_settingsController->Initialize(m_settingsManager.get(), m_cameraController);
    m_settingsController->SetBackCallback([this]() { m_state = MenuState::Options; });

    // Load configuration
    LoadConfiguration();

    // Initialize maps
    if (m_mapSelector)
    {
        m_mapSelector->InitializeMaps();
    }
}

void Menu::Initialize(Engine *engine)
{
    m_engine = engine;

    HandleKeyboardNavigation();

    // Handle pending actions
    HandlePendingActions();
}

void Menu::Update()
{
    // Update console if open
    if (m_consoleManager && m_consoleManager->IsConsoleOpen())
    {
        // Console handles its own update/input
    }
    else
    {
        // Handle menu navigation
        HandleKeyboardNavigation();
    }

    // Handle pending actions (settings, etc.)
    HandlePendingActions();
}

void Menu::Render()
{

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Set up main window for the menu (fullscreen)
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

void Menu::BeginFrame()
{
    rlImGuiBegin();
}

void Menu::EndFrame()
{
    rlImGuiEnd();
}

void Menu::SetupStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO &io = ImGui::GetIO();

    // Load Gantari Font
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/Gantari/static/Gantari-Regular.ttf";
    if (std::filesystem::exists(fontPath))
    {
        // Increase base font size for sharper text (was 20.0f)
        ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 32.0f);
        if (font)
        {
            TraceLog(LOG_INFO, "[Menu] Loaded custom font: %s", fontPath.c_str());
            // Important: Notify rlImGui to rebuild the font texture
            // Assuming rlImGuiReloadFonts() or similar is available, or we just rely on standard
            // flow. Since we don't have direct access to rlImGui internal reload easily without
            // including it, we'll see if it works. Standard ImGui requires texture rebuild. If
            // rlImGui is used, we usually need to call rlImGuiReloadFonts(). Let's try to verify if
            // we can include it.
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "[Menu] Custom font not found: %s", fontPath.c_str());
    }

    // Customize colors for a more modern look
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.ChildRounding = 8.0f;

    // Improved spacing and sizing
    style.WindowPadding = ImVec2(16.0f, 16.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(12.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 20.0f;
    style.GrabMinSize = 20.0f;

    // Modern scrollbar and tab styling
    style.ScrollbarSize = 16.0f;
    style.TabBorderSize = 0.0f;

    // Set up colors with a modern dark theme
    ImVec4 *colors = style.Colors;

    // Window and background colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);

    // Title bar colors
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

    // Button colors
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 0.8f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.9f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

    // Frame colors
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);

    // Slider colors
    colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);

    // Text colors
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    // Border colors
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.24f);

    // Scrollbar colors
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    // Tab colors
    colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

    // Header colors
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
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
        if (m_settingsController)
            m_settingsController->RenderVideoSettings();
        break;
    case MenuState::Audio:
        if (m_settingsController)
            m_settingsController->RenderAudioSettings();
        break;
    case MenuState::Controls:
        if (m_settingsController)
            m_settingsController->RenderControlSettings();
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
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title section
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::PushFont(nullptr); // Use default font but with larger size
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("CHAINED DECOS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::PopStyleColor();

    // Subtitle
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::NAME_FONT_SIZE) / 32.0f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Parkour Adventure");
    ImGui::SetWindowFontScale(1.0f);

    // Menu buttons container
    float startY = MenuConstants::TOP_MARGIN + 100;
    float currentY = startY;

    if (m_gameInProgress)
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
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::INSTRUCTIONS_FONT_SIZE) / 32.0f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "[~] Console | [F12] Screenshot | [ESC] Back");
    ImGui::SetWindowFontScale(1.0f);
}

void Menu::RenderGameModeMenu()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("GAME MODE SELECTION");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Menu buttons - centered vertically (2 buttons total)
    const int buttonCount = 2;
    const float totalHeight = (buttonCount * buttonHeight) + ((buttonCount - 1) * spacing);
    float startY = centerY - totalHeight / 2.0f;
    float currentY = startY;

    // Single Player button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Single Player", MenuAction::None, ImVec2(buttonWidth, buttonHeight)))
    {
        // Go to map selection for single player
        m_state = MenuState::MapSelection;
    }
    currentY += buttonHeight + spacing;

    // Multi Player button (disabled)
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
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
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("OPTIONS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Options buttons - centered vertically (3 buttons total)
    const int buttonCount = 3;
    const float totalHeight = (buttonCount * buttonHeight) + ((buttonCount - 1) * spacing);
    float startY = centerY - totalHeight / 2.0f;
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

    // Back button
    ImGui::SetCursorPos(ImVec2(80, windowSize.y - 60));
    RenderBackButton();
}

void Menu::RenderMapSelection()
{
    if (m_mapSelector)
    {
        m_mapSelector->RenderMapSelectionWindow();

        // Start Game button
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const float centerX = windowSize.x * 0.5f;
        ImGui::SetCursorPos(ImVec2(centerX - 160, windowSize.y - 100));
        RenderActionButton("Start Game with Selected Map", MenuAction::StartGameWithMap,
                           ImVec2(320, 50));
    }
    else
    {
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const float centerX = windowSize.x * 0.5f;
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
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("CREDITS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Credits content
    float startY = centerY - 120;
    float currentY = startY;
    float sectionSpacing = 60.0f;
    float labelSpacing = 30.0f;

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
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
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
        m_engine->Shutdown();
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
        case MenuAction::ApplyAudioSettings:
        case MenuAction::ApplyControlSettings:
            if (m_settingsController)
                m_settingsController->ApplyPendingSettings();
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
    ImVec2 buttonSize(120.0f, 40.0f); // Same size as Apply button
    if (width > 0)
    {
        buttonSize.x = width;
    }

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

// State management
void Menu::SetGameInProgress(bool inProgress)
{
    m_gameInProgress = inProgress;
}

bool Menu::IsGameInProgress() const
{
    return m_gameInProgress;
}

MenuAction Menu::ConsumeAction()
{
    MenuAction action = m_pendingAction;
    m_pendingAction = MenuAction::None;
    return action;
}

MenuState Menu::GetState() const
{
    return m_state;
}

void Menu::SetState(MenuState state)
{
    m_state = state;
}

// Navigation methods
void Menu::ShowMainMenu()
{
    m_state = MenuState::Main;
}
void Menu::ShowOptionsMenu()
{
    m_state = MenuState::Options;
}
void Menu::ShowGameModeMenu()
{
    m_state = MenuState::GameMode;
}
void Menu::ShowMapSelection()
{
    m_state = MenuState::MapSelection;
}
void Menu::ShowAudioMenu()
{
    m_state = MenuState::Audio;
}
void Menu::ShowVideoMenu()
{
    m_state = MenuState::Video;
}
void Menu::ShowControlsMenu()
{
    m_state = MenuState::Controls;
}
void Menu::ShowCredits()
{
    m_state = MenuState::Credits;
}
void Menu::ShowMods()
{
    m_state = MenuState::Mods;
}
void Menu::ShowConfirmExit()
{
    m_state = MenuState::ConfirmExit;
}

// Apply pending settings
void Menu::ApplyPendingSettings()
{
    // Delegate to settings controller
    if (m_settingsController)
    {
        m_settingsController->ApplyPendingSettings();
    }

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
    // Delegate to MapSelector
    if (m_mapSelector)
    {
        m_mapSelector->InitializeMaps();
    }
}

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
        // Apply initial audio settings to AudioManager
        m_settingsManager->ApplyAudioSettings();
    }
}

// Action management
void Menu::SetAction(MenuAction action)
{
    m_pendingAction = action;
}

MenuAction Menu::GetAction() const
{
    return m_pendingAction;
}

void Menu::ResetAction()
{
    m_pendingAction = MenuAction::None;
}

// Console functionality
void Menu::ToggleConsole()
{
    if (m_consoleManager)
    {
        m_consoleManager->ToggleConsole();
    }
}

bool Menu::IsConsoleOpen() const
{
    return m_consoleManager && m_consoleManager->IsConsoleOpen();
}

// Helper methods
void Menu::HandleAction(MenuAction action)
{
    m_pendingAction = action;
}

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

void Menu::SetResumeButtonOn(bool status)
{
    m_addResumeButton = status;
}

bool Menu::GetResumeButtonStatus() const
{
    return m_addResumeButton;
}

ConsoleManager *Menu::GetConsoleManager() const
{
    return m_consoleManager.get();
}

// Apply camera sensitivity to CameraController (Dependency Injection)
void Menu::SetCameraController(ICameraSensitivityController *controller)
{
    m_cameraController = controller;
}

[[nodiscard]] SettingsManager *Menu::GetSettingsManager() const
{
    return m_settingsManager.get();
}
bool Menu::IsOpen() const
{
    return m_state != MenuState::GameMode;
}
void Menu::Show()
{
    m_state = MenuState::Main;
}
void Menu::Hide()
{
    m_state = MenuState::GameMode;
}
bool Menu::ShouldStartGame() const
{
    return m_action == MenuAction::StartGame || m_action == MenuAction::ResumeGame;
}
