#include "Menu.h"
#include "MenuConstants.h"
#include "Settings/SettingsManager.h"
#include "project/chaineddecos/events/MenuEvent.h"

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
    : m_state(MenuState::Main), m_pendingAction(MenuAction::None), m_gameInProgress(false),
      m_settingsManager(std::make_unique<SettingsManager>()),
      m_consoleManager(std::make_unique<ConsoleManager>()),
      m_mapSelector(std::make_unique<MapSelector>()), m_presenter(std::make_unique<MenuPresenter>())
{
    // Initialize MenuSettingsController
    m_settingsController = std::make_unique<MenuSettingsController>();
    m_settingsController->Initialize(m_settingsManager.get(), m_cameraController);
    m_settingsController->SetBackCallback([this]() { SetState(MenuState::Options); });

    // Setup presenter action callback
    m_presenter->SetActionCallback(
        [this](MenuAction action)
        {
            if (action == MenuAction::BackToMainMenu)
                m_state = MenuState::Main;
            else
                m_pendingAction = action;
        });

    LoadConfiguration();
    if (m_mapSelector)
        m_mapSelector->InitializeMaps();
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
    if (m_presenter)
        m_presenter->SetupStyle();
}

void Menu::RenderMenuState()
{
    if (!m_presenter)
        return;

    switch (m_state)
    {
    case MenuState::Main:
        m_presenter->RenderMainMenu(m_gameInProgress, m_addResumeButton);
        break;
    case MenuState::GameMode:
        m_presenter->RenderGameModeMenu();
        break;
    case MenuState::MapSelection:
        if (m_mapSelector)
            m_mapSelector->RenderMapSelectionWindow();
        break;
    case MenuState::Options:
        m_presenter->RenderOptionsMenu();
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
        m_presenter->RenderCreditsScreen();
        break;
    case MenuState::Mods:
        m_presenter->RenderModsScreen();
        break;
    case MenuState::ConfirmExit:
        m_presenter->RenderConfirmExitDialog();
        break;
    default:
        m_presenter->RenderMainMenu(m_gameInProgress, m_addResumeButton);
        break;
    }

    if (IsConsoleOpen())
        m_presenter->RenderConsoleOverlay(m_consoleManager.get());
}

// Helper methods
void Menu::HandlePendingActions()
{
    if (m_pendingAction != MenuAction::None)
    {
        switch (m_pendingAction)
        {
        case MenuAction::StartGame:
            m_state = MenuState::GameMode;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::SinglePlayer:
            m_state = MenuState::MapSelection;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::ResumeGame:
        case MenuAction::ExitGame:
        case MenuAction::StartGameWithMap:
            if (m_eventCallback)
            {
                ChainedDecos::MenuEvent e(m_pendingAction);
                m_eventCallback(e);
            }
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::OpenOptions:
            m_state = MenuState::Options;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::OpenCredits:
            m_state = MenuState::Credits;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::OpenMods:
            m_state = MenuState::Mods;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::OpenVideoMode:
            m_state = MenuState::Video;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::OpenAudio:
            m_state = MenuState::Audio;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::OpenControls:
            m_state = MenuState::Controls;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::BackToMainMenu:
            m_state = MenuState::Main;
            m_pendingAction = MenuAction::None;
            break;
        case MenuAction::ApplyVideoSettings:
        case MenuAction::ApplyAudioSettings:
        case MenuAction::ApplyControlSettings:
            if (m_settingsController)
                m_settingsController->ApplyPendingSettings();
            m_pendingAction = MenuAction::None;
            break;
        default:
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

    if (m_state == MenuState::MapSelection && m_mapSelector && m_mapSelector->HasMaps())
    {
        m_mapSelector->HandleKeyboardNavigation();
        if (IsKeyPressed(KEY_ENTER))
        {
            m_pendingAction = MenuAction::StartGameWithMap;
        }
    }
}

// Helper methods removal (functionality moved)

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

// State title functionality moved to presenter

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
    if (m_settingsController)
    {
        m_settingsController->SetCameraController(controller);
    }
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
