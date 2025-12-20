#include "Menu.h"
#include "MenuConstants.h"
#include "Settings/SettingsManager.h"
#include <filesystem>
#include <imgui.h>
#include <memory>
#include <raylib.h>
#include <string>

// Concrete screens
#include "screens/ConfirmExitScreen.h"
#include "screens/CreditsScreen.h"
#include "screens/MainMenuScreen.h"
#include "screens/MapSelectionScreen.h"
#include "screens/OptionsScreen.h"
#include "screens/SettingsScreens.h"

Menu::Menu()
    : m_state(MenuState::Main), m_gameInProgress(false), m_showDemoWindow(false),
      m_showStyleEditor(false), m_settingsManager(std::make_unique<SettingsManager>()),
      m_consoleManager(std::make_unique<ConsoleManager>()),
      m_mapSelector(std::make_unique<MapSelector>())
{
    // Initialize screens folder
    std::filesystem::create_directories(std::string(PROJECT_ROOT_DIR) + "/resources/maps");

    // Initialize MenuSettingsController
    m_settingsController = std::make_unique<MenuSettingsController>();
    m_settingsController->Initialize(m_settingsManager.get(), nullptr);
    m_settingsController->SetBackCallback([this]() { ShowOptionsMenu(); });

    // Load configuration
    LoadConfiguration();

    // Initialize maps
    if (m_mapSelector)
    {
        m_mapSelector->InitializeMaps();
    }

    // Set initial screen
    ShowMainMenu();
}

void Menu::Initialize(Engine *engine)
{
    m_engine = engine;

    // Set initial screen if not set (redundant with constructor but safe)
    if (!m_currentScreen)
    {
        ShowMainMenu();
    }

    // Initialize UI style
    SetupStyle();

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

        // Delegate to current screen
        if (m_currentScreen)
        {
            m_currentScreen->Update();
            m_currentScreen->HandleInput();
        }
    }

    // Handle pending actions (settings, etc.)
    HandlePendingActions();
}

void Menu::Render()
{
    // The ImGui frame is now managed by GameApplication or EngineApplication
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("MenuWindow", nullptr, windowFlags))
    {
        // Check for console first (always on top)
        if (m_consoleManager && m_consoleManager->IsConsoleOpen())
        {
            RenderConsoleOverlay();
        }
        else
        {
            // Render current screen
            if (m_currentScreen)
            {
                m_currentScreen->Render();
            }
        }

        // Debug overlays (if enabled)
        if (m_showDemoWindow)
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        if (m_showStyleEditor)
            ImGui::ShowStyleEditor();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void Menu::SetScreen(std::unique_ptr<IMenuScreen> screen)
{
    m_currentScreen = std::move(screen);
    if (m_currentScreen)
    {
        m_currentScreen->Initialize(this);
    }
}

void Menu::ShowMainMenu()
{
    m_state = MenuState::Main;
    SetScreen(std::make_unique<MainMenuScreen>());
}

void Menu::ShowOptionsMenu()
{
    m_state = MenuState::Options;
    SetScreen(std::make_unique<OptionsScreen>());
}

void Menu::ShowMapSelection()
{
    m_state = MenuState::MapSelection;
    SetScreen(std::make_unique<MapSelectionScreen>());
}

void Menu::ShowAudioMenu()
{
    m_state = MenuState::Audio;
    SetScreen(std::make_unique<AudioSettingsScreen>());
}

void Menu::ShowVideoMenu()
{
    m_state = MenuState::Video;
    SetScreen(std::make_unique<VideoSettingsScreen>());
}

void Menu::ShowControlsMenu()
{
    m_state = MenuState::Controls;
    SetScreen(std::make_unique<ControlSettingsScreen>());
}

void Menu::ShowCredits()
{
    m_state = MenuState::Credits;
    SetScreen(std::make_unique<CreditsScreen>());
}

void Menu::ShowConfirmExit()
{
    m_state = MenuState::ConfirmExit;
    SetScreen(std::make_unique<ConfirmExitScreen>());
}

void Menu::HandleKeyboardNavigation()
{
    if (IsKeyPressed(KEY_GRAVE))
    {
        TraceLog(LOG_INFO, "Menu::HandleKeyboardNavigation() - Console toggle key pressed");
        ToggleConsole();
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        // Global escape handling for back navigation
        if (m_state != MenuState::Main)
        {
            if (m_state == MenuState::Video || m_state == MenuState::Audio ||
                m_state == MenuState::Controls)
            {
                ShowOptionsMenu();
            }
            else
            {
                ShowMainMenu();
            }
        }
    }
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
        ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 24.0f);
        if (font)
        {
            TraceLog(LOG_INFO, "[Menu] Loaded custom font: %s", fontPath.c_str());
            io.FontDefault = font;
        }
    }

    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.25f, 0.7f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.4f, 0.8f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.4f, 0.6f, 1.0f);
}

bool Menu::RenderActionButton(const char *label, ChainedDecos::MenuEventType eventType,
                              const ImVec2 &size)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleVar();

    if (clicked && eventType != ChainedDecos::MenuEventType::None)
    {
        TraceLog(LOG_INFO, "Menu::RenderActionButton() - Button '%s' clicked, event: %d", label,
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
    ImVec2 buttonSize(120.0f, 40.0f);
    if (width > 0)
        buttonSize.x = width;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

    bool clicked = ImGui::Button("Back", buttonSize);
    ImGui::PopStyleColor(4);

    if (clicked)
    {
        ShowMainMenu();
        return true;
    }
    return false;
}

void Menu::RenderSectionHeader(const char *title, const char *subtitle) const
{
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), title);
    if (subtitle)
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), subtitle);
}

void Menu::RenderMenuHint(const char *text) const
{
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), text);
}

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

void Menu::DispatchEvent(ChainedDecos::MenuEventType type, const std::string &data)
{
    if (m_eventCallback)
    {
        m_eventCallback({type, data});
    }
}

void Menu::SetEventCallback(const MenuEventCallback &callback)
{
    m_eventCallback = callback;
}

void Menu::ToggleConsole()
{
    if (m_consoleManager)
        m_consoleManager->ToggleConsole();
}

bool Menu::IsConsoleOpen() const
{
    return m_consoleManager && m_consoleManager->IsConsoleOpen();
}

void Menu::RenderConsoleOverlay()
{
    if (m_consoleManager)
        m_consoleManager->RenderConsole();
}

void Menu::ApplyPendingSettings()
{
    if (m_settingsController)
        m_settingsController->ApplyPendingSettings();
}

void Menu::SaveConfiguration()
{
    if (m_settingsManager)
        m_settingsManager->SaveSettings();
}

void Menu::LoadConfiguration()
{
    if (m_settingsManager)
        m_settingsManager->LoadSettings();
}

std::optional<MapInfo> Menu::GetSelectedMap() const
{
    if (m_mapSelector && m_mapSelector->GetSelectedMap())
        return *(m_mapSelector->GetSelectedMap());
    return std::nullopt;
}

std::string Menu::GetSelectedMapName() const
{
    auto map = GetSelectedMap();
    return map ? map->name : "";
}

void Menu::InitializeMaps()
{
    if (m_mapSelector)
        m_mapSelector->InitializeMaps();
}

bool Menu::IsOpen() const
{
    return true;
}
void Menu::Show()
{
    ShowMainMenu();
}
void Menu::Hide()
{
}

ConsoleManager *Menu::GetConsoleManager() const
{
    return m_consoleManager.get();
}
SettingsManager *Menu::GetSettingsManager() const
{
    return m_settingsManager.get();
}

void Menu::SetCameraController(ICameraSensitivityController *controller)
{
    m_cameraController = controller;
    if (m_settingsController)
        m_settingsController->SetCameraController(controller);
}

void Menu::HandlePendingActions()
{
    // Implementation for handling delayed actions if any
}
