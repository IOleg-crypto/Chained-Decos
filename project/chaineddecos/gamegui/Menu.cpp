#include "Menu.h"
#include "MenuConstants.h"
#include "Settings/SettingsManager.h"

#include "components/physics/collision/System/CollisionSystem.h"
#include "core/Engine.h"
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
    : m_state(MenuState::Main), m_gameInProgress(false), m_showDemoWindow(false),
      m_showStyleEditor(false), m_settingsManager(std::make_unique<SettingsManager>()),
      m_consoleManager(std::make_unique<ConsoleManager>()),
      m_mapSelector(std::make_unique<MapSelector>())
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
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground; // Transparency for premium look

    ImGui::Begin("Game Menu", nullptr, windowFlags);

    // Add a dark overlay gradient or background
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2((float)screenWidth, (float)screenHeight),
                            ImColor(10, 10, 15, 200)); // Deep dark blue-black tint

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
        // Increase base font size for sharper text (was 32.0f)
        ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 24.0f);
        if (font)
        {
            TraceLog(LOG_INFO, "[Menu] Loaded custom font: %s", fontPath.c_str());
            io.FontDefault = font; // Set as default

            // Rebuild font texture for rlImGui
            // This is critical for raylib + imfont integration
            unsigned char *pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            // Note: In rlImGui, the texture is managed internally.
            // We usually need to call rlImGuiReloadFonts() if it's exposed,
            // or perform a similar operation.
            // Assuming rlImGui handles this if called before rlImGuiBegin.
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "[Menu] Custom font not found: %s", fontPath.c_str());
    }

    // Customize colors for a "Premium" look
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.PopupRounding = 12.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding = 12.0f;
    style.ChildRounding = 12.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 0.0f;

    // Improved spacing and sizing
    style.WindowPadding = ImVec2(24.0f, 24.0f);
    style.FramePadding = ImVec2(16.0f, 10.0f);
    style.ItemSpacing = ImVec2(12.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(10.0f, 8.0f);
    style.IndentSpacing = 25.0f;
    style.GrabMinSize = 22.0f;

    // Premium Color Palette (Deep Ocean / Gold Accents)
    ImVec4 *colors = style.Colors;

    ImVec4 accentColor = ImVec4(1.0f, 0.75f, 0.3f, 1.0f); // Soft Gold
    ImVec4 accentHover = ImVec4(1.0f, 0.85f, 0.5f, 1.0f);
    ImVec4 accentActive = ImVec4(0.9f, 0.65f, 0.2f, 1.0f);

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.16f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.25f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 0.67f);

    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = accentActive;

    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive] = accentColor;

    colors[ImGuiCol_SliderGrab] = accentColor;
    colors[ImGuiCol_SliderGrabActive] = accentHover;

    colors[ImGuiCol_CheckMark] = accentColor;
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
        if (RenderActionButton("Resume Game", ChainedDecos::MenuEventType::ResumeGame,
                               ImVec2(buttonWidth, buttonHeight)))
        {
            m_state = MenuState::Resume;
        }
        currentY += buttonHeight + spacing;
    }
    // Start Game button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Start Game", ChainedDecos::MenuEventType::StartGame,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::GameMode;
    }
    currentY += buttonHeight + spacing;

    // Options button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Options", ChainedDecos::MenuEventType::OpenOptions,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Options;
    }
    currentY += buttonHeight + spacing;

    // Credits button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Credits", ChainedDecos::MenuEventType::OpenCredits,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Credits;
    }
    currentY += buttonHeight + spacing;

    // Mods button removed as per user request

    // Exit Game button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Exit Game", ChainedDecos::MenuEventType::ExitGame,
                           ImVec2(buttonWidth, buttonHeight)))
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
    if (RenderActionButton("Single Player", ChainedDecos::MenuEventType::None,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        // Go to map selection for single player
        m_state = MenuState::MapSelection;
    }
    currentY += buttonHeight + spacing;

    // Multi Player button (disabled)
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    ImGui::BeginDisabled(true);
    if (RenderActionButton("Multi Player(Coming soon)", ChainedDecos::MenuEventType::None,
                           ImVec2(buttonWidth, buttonHeight)))
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
    if (RenderActionButton("Video Settings", ChainedDecos::MenuEventType::OpenVideoSettings,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Video;
    }
    currentY += buttonHeight + spacing;

    // Audio Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Audio Settings", ChainedDecos::MenuEventType::OpenAudioSettings,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        m_state = MenuState::Audio;
    }
    currentY += buttonHeight + spacing;

    // Control Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Control Settings", ChainedDecos::MenuEventType::OpenControlSettings,
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
        m_mapSelector->RenderMapSelectionImGui();

        // Start Game button - centered at the bottom
        const ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        float btnWidth = 300.0f;
        float btnHeight = 50.0f;
        ImGui::SetCursorPos(ImVec2((windowSize.x - btnWidth) * 0.5f, windowSize.y - 70.0f));

        if (RenderActionButton("START GAME WITH SELECTED MAP",
                               ChainedDecos::MenuEventType::StartGameWithMap,
                               ImVec2(btnWidth, btnHeight)))
        {
            // Action handled by RenderActionButton
        }

        // Back button - bottom left
        ImGui::SetCursorPos(ImVec2(40, windowSize.y - 70.0f));
        if (RenderBackButton(120.0f))
        {
            // Action handled by RenderBackButton
        }
    }
    else
    {
        const ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        ImGui::SetCursorPos(ImVec2((windowSize.x - 200) * 0.5f, windowSize.y * 0.5f));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No maps available");

        ImGui::SetCursorPos(ImVec2(40, windowSize.y - 70.0f));
        RenderBackButton(120.0f);
    }
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

// RenderModsScreen removed

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
void Menu::DispatchEvent(ChainedDecos::MenuEventType type, const std::string &data)
{
    if (m_eventCallback)
    {
        m_eventCallback(ChainedDecos::MenuEvent(type, data));
    }
}

void Menu::SetEventCallback(const MenuEventCallback &callback)
{
    m_eventCallback = callback;
}

void Menu::HandlePendingActions()
{
    // Settings actions are still internal to Menu/MenuSettingsController
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

bool Menu::RenderActionButton(const char *label, ChainedDecos::MenuEventType eventType,
                              const ImVec2 &size)
{
    // Enhanced button styling - slightly more premium
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleVar();

    if (clicked && eventType != ChainedDecos::MenuEventType::None)
    {
        TraceLog(LOG_INFO,
                 "Menu::RenderActionButton() - Button '%s' clicked, dispatching event: %d", label,
                 static_cast<int>(eventType));

        std::string data = "";
        if (eventType == ChainedDecos::MenuEventType::StartGameWithMap)
        {
            data = GetSelectedMapName();
        }

        DispatchEvent(eventType, data);
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
// ShouldStartGame and ResetAction removed as they were removed from IMenu
