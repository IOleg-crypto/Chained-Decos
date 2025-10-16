#include "Menu.h"
#include "Engine/Engine.h"
#include <raylib.h>
#include <iostream>

// Core menu functionality
Menu::Menu()
{
    // Load configuration file
    LoadSettings();

    // Load Alan Sans font for menu text (only if graphics are available)
    const std::string alanSansFontPath = PROJECT_ROOT_DIR "/resources/font/AlanSans.ttf";

    // Check if we're in a headless environment (no graphics)
    bool graphicsAvailable = true;
    try {
        // Try to detect if graphics are available by checking if we can get screen dimensions
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        if (screenWidth <= 0 || screenHeight <= 0) {
            graphicsAvailable = false;
        }
    } catch (...) {
        graphicsAvailable = false;
    }

    if (graphicsAvailable) {
        m_font = LoadFontEx(alanSansFontPath.c_str(), 64, nullptr, 0);

        if (m_font.texture.id == 0)
        {
            TraceLog(LOG_WARNING, "Menu::Menu() - Failed to load Alan Sans font: %s, using default font",
                     alanSansFontPath.c_str());
            m_font = GetFontDefault();
        }
        else
        {
            // Set texture filter for smooth scaling
            SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
            TraceLog(LOG_INFO, "Menu::Menu() - Alan Sans font loaded successfully: %s",
                     alanSansFontPath.c_str());
        }
    } else {
        // Headless environment - use default font without loading
        TraceLog(LOG_INFO, "Menu::Menu() - Headless environment detected, using default font");
        m_font = GetFontDefault();
    }

    // Main Menu - Resume Game will be added dynamically if game is in progress
    m_mainMenu = {{"Start Game", MenuAction::StartGame},
                  {"Options", MenuAction::OpenOptions},
                  {"Mods", MenuAction::OpenMods},
                  {"Credits", MenuAction::OpenCredits},
                  {"Quit", MenuAction::ExitGame}};

    // Options Menu
    m_optionsMenu = {{"Video", MenuAction::OpenVideoMode},
                      {"Audio", MenuAction::OpenAudio},
                      {"Controls", MenuAction::OpenControls},
                      {"Gameplay", MenuAction::OpenGameplay},
                      {"Back", MenuAction::BackToMainMenu}};

    // Game Mode Menu
    m_SetGameMode = {{"Singleplayer", MenuAction::SinglePlayer},
                     {"Multiplayer", MenuAction::MultiPlayer},
                     {"Back", MenuAction::BackToMainMenu}};

    // Video Options
    m_videoOptions = {
        {"Resolution", {"800x600", "1280x720", "1360x768", "1920x1080", "2560x1440"}, 0},
        {"Aspect Ratio", {"16:9", "4:3", "21:9"}, 0},
        {"Display Mode", {"Windowed", "Fullscreen", "Borderless"}, 0},
        {"VSync", {"Off", "On"}, 1},
        {"Target FPS", {"30", "60", "120", "144", "165", "180", "240", "Unlimited"}, 1},
        {"Back", {}, 0}};

    // Audio Menu with volume controls
    m_audioMenu = {{"Master Volume", MenuAction::AdjustMasterVolume},
                   {"Music Volume", MenuAction::AdjustMusicVolume},
                   {"SFX Volume", MenuAction::AdjustSFXVolume},
                   {"Mute Audio", MenuAction::ToggleMute},
                   {"Back", MenuAction::BackToMainMenu}};

    // Controls Menu with proper actions
    m_controlsMenu = {{"Rebind Keys", MenuAction::OpenKeyBinding},
                       {"Mouse Sensitivity", MenuAction::AdjustMouseSensitivity},
                       {"Invert Y Axis", MenuAction::ToggleInvertY},
                       {"Controller Support", MenuAction::ToggleController},
                       {"Back", MenuAction::BackToMainMenu}};

    // Gameplay Options - using MenuOption structure like video settings
    m_gameplayOptions = {
        {"Difficulty", {"Easy", "Medium", "Hard"}, 1},
        {"Timer", {"Off", "On"}, 1},
        {"Checkpoints", {"Off", "On"}, 1},
        {"Auto Save", {"Off", "On"}, 1},
        {"Speedrun Mode", {"Off", "On"}, 0},
        {"Back", {}, 0}
    };


    m_currentMenu = &m_mainMenu;
    m_buttonScales.assign(m_currentMenu->size(), 1.0f);

    // Ensure we start at the main menu state
    m_state = MenuState::Main;
}

float Menu::Lerp(float a, float b, float t) const { return a + (b - a) * t; }

MenuAction Menu::GetAction() const { return m_action; }

void Menu::ResetAction() { m_action = MenuAction::None; }

void Menu::GetEngine(Engine *engine) { m_engine = engine; }

// Helper function to get current menu with dynamic items
const std::vector<MenuItem>* Menu::GetCurrentMenuWithDynamicItems() const
{
    if (m_state == MenuState::Main && m_gameInProgress)
    {
        // Create a static dynamic menu for main menu with resume option
        static std::vector<MenuItem> dynamicMainMenu;
        dynamicMainMenu = {{"Resume Game", MenuAction::ResumeGame},
                          {"Start Game", MenuAction::StartGame},
                          {"Options", MenuAction::OpenOptions},
                          {"Mods", MenuAction::OpenMods},
                          {"Credits", MenuAction::OpenCredits},
                          {"Quit", MenuAction::ExitGame}};
        return &dynamicMainMenu;
    }
    return m_currentMenu;
}

void Menu::SetGameInProgress(bool inProgress)
{
    m_gameInProgress = inProgress;
    TraceLog(LOG_INFO, "Menu::SetGameInProgress() - Game state set to: %s", inProgress ? "IN PROGRESS" : "NOT IN PROGRESS");
}

void Menu::Update()
{
    // // If game is in progress and we're not in a submenu, ensure we're in main menu to show resume option
    // if (m_gameInProgress && m_state != MenuState::Options && m_state != MenuState::Gameplay &&
    //     m_state != MenuState::ParkourControls && m_state != MenuState::Video &&
    //     m_state != MenuState::Audio && m_state != MenuState::Controls &&
    //     m_state != MenuState::Credits && m_state != MenuState::Mods &&
    //     m_state != MenuState::MapSelection && m_state != MenuState::ConfirmExit)
    // {
    //     m_state = MenuState::Main;
    // }

    // Update current menu based on state
    switch (m_state)
    {
    case MenuState::Main:
        m_currentMenu = &m_mainMenu;
        break;
    case MenuState::Options:
        m_currentMenu = &m_optionsMenu;
        break;
    case MenuState::GameMode:
        m_currentMenu = &m_SetGameMode;
        break;
    case MenuState::MapSelection:
        // Map selection doesn't use the regular menu system
        m_currentMenu = nullptr;
        break;
    case MenuState::Audio:
        m_currentMenu = &m_audioMenu;
        // Initialize audio menu selection if needed
        if (m_selected >= static_cast<int>(m_audioMenu.size()))
            m_selected = 0;
        break;
    case MenuState::Controls:
        m_currentMenu = &m_controlsMenu;
        break;
    case MenuState::Gameplay:
        // Use options-based system like video settings
        // Initialize gameplay menu selection if needed
        if (m_selected < 0 || m_selected >= static_cast<int>(m_gameplayOptions.size()))
            m_selected = 0;
        HandleGameplayNavigation();
        break;
    default:
        m_currentMenu = nullptr;
        break;
    }

    if (m_currentMenu && (m_selected < 0 || m_selected >= static_cast<int>(m_currentMenu->size())))
        m_selected = 0;

    // Handle input
    if (m_state == MenuState::Video)
        HandleVideoNavigation();
    else if (m_state == MenuState::MapSelection)
        HandleMapSelection();
    else if (m_state != MenuState::Credits && m_state != MenuState::Mods &&
             m_state != MenuState::ConfirmExit)
        HandleKeyboardNavigation();

    if (m_state == MenuState::Credits || m_state == MenuState::Mods)
    {
        if (IsKeyPressed(KEY_ESCAPE))
            m_state = MenuState::Main;
    }

    // Handle console toggle
    if (IsKeyPressed(KEY_GRAVE)) // ~ key
    {
        ToggleConsole();
    }

    HandleConsoleInput();
    HandleMouseSelection();

    if (m_state == MenuState::ConfirmExit)
        HandleConfirmExit();

    ExecuteAction();
}

void Menu::ExecuteAction()
{
    if (m_action == MenuAction::None)
        return;

    switch (m_action)
    {
    case MenuAction::StartGame:
        m_state = MenuState::GameMode;
        ResetAction();
        break;
    case MenuAction::ResumeGame:
        TraceLog(LOG_INFO, "Menu::ExecuteAction() - Resuming game...");
        // Resume game will be handled by Game class
        ResetAction();
        break;
    case MenuAction::SinglePlayer:
        TraceLog(LOG_INFO, "Menu::ExecuteAction() - SinglePlayer selected, initializing map selection...");
        m_state = MenuState::MapSelection;
        InitializeMaps();
        TraceLog(LOG_INFO, "Menu::ExecuteAction() - Map selection initialized with %d maps", m_availableMaps.size());
        ResetAction();
        break;
    case MenuAction::MultiPlayer:
        std::cout << "Selected mode: Multiplayer\n";
        break;
    case MenuAction::OpenOptions:
        m_state = MenuState::Options;
        ResetAction();
        break;
    case MenuAction::OpenVideoMode:
        m_state = MenuState::Video;
        ResetAction();
        break;
    case MenuAction::OpenAudio:
        m_state = MenuState::Audio;
        ResetAction();
        break;
    case MenuAction::OpenControls:
        m_state = MenuState::Controls;
        ResetAction();
        break;
    case MenuAction::OpenGameplay:
        m_state = MenuState::Gameplay;
        ResetAction();
        break;
    case MenuAction::AdjustMasterVolume:
        // Volume adjustment is handled in real-time via keyboard input
        break;
    case MenuAction::AdjustMusicVolume:
        // Volume adjustment is handled in real-time via keyboard input
        break;
    case MenuAction::AdjustSFXVolume:
        // Volume adjustment is handled in real-time via keyboard input
        break;
    case MenuAction::ToggleMute:
        m_audioMuted = !m_audioMuted;
        // TODO: Apply mute to audio system
        AddConsoleOutput(m_audioMuted ? "Audio muted" : "Audio unmuted");
        ResetAction();
        break;
    case MenuAction::OpenKeyBinding:
        AddConsoleOutput("Key binding not implemented yet");
        ResetAction();
        break;
    case MenuAction::AdjustMouseSensitivity:
        // Sensitivity adjustment is handled in real-time via keyboard input
        break;
    case MenuAction::ToggleInvertY:
        m_invertYAxis = !m_invertYAxis;
        AddConsoleOutput(m_invertYAxis ? "Y-axis inverted" : "Y-axis normal");
        ResetAction();
        break;
    case MenuAction::ToggleController:
        m_controllerSupport = !m_controllerSupport;
        AddConsoleOutput(m_controllerSupport ? "Controller support enabled" : "Controller support disabled");
        ResetAction();
        break;
    case MenuAction::OpenCredits:
        m_state = MenuState::Credits;
        ResetAction();
        break;
    case MenuAction::OpenMods:
        m_state = MenuState::Mods;
        ResetAction();
        break;
    case MenuAction::BackToMainMenu:
        m_state = (m_state == MenuState::Video || m_state == MenuState::Audio ||
                   m_state == MenuState::Controls || m_state == MenuState::Gameplay ||
                   m_state == MenuState::ParkourControls)
                      ? MenuState::Options
                      : MenuState::Main;
        ResetAction();
        break;
    case MenuAction::ExitGame:
        if (m_state == MenuState::ConfirmExit)
        {
            // User confirmed exit, actually exit the game
            if (m_engine)
                m_engine->RequestExit();
            // Set game state to not in progress when exiting
            m_gameInProgress = false;
            ResetAction();
        }
        else
        {
            // Show confirmation dialog
            m_state = MenuState::ConfirmExit;
            // Don't reset action, we need it for confirmation
        }
        break;
    case MenuAction::StartGameWithMap:
        // This action is handled by the Game system, don't reset it here
        // The Game system will handle the transition and reset the action
        break;
    default:
        ResetAction();
        break;
    }
}

void Menu::LoadSettings()
{
    // Load configuration from file (try current directory first, then build directory)
    try
    {
        if (!m_config.LoadFromFile("game.cfg"))
        {
            // Try loading from build directory if not found in current directory
            if (!m_config.LoadFromFile("build/game.cfg"))
            {
                TraceLog(LOG_WARNING, "Menu::Menu() - Could not load game.cfg, will use default settings");
            }
        }
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Menu::Menu() - Exception while loading config: %s", e.what());
        TraceLog(LOG_INFO, "Menu::Menu() - Continuing with default settings");
    }

    // Apply loaded settings to the game
    int width, height;
    m_config.GetResolution(width, height);
    SetWindowSize(width, height);

    if (m_config.IsFullscreen())
    {
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }

    if (m_config.IsVSync())
    {
        SetWindowState(FLAG_VSYNC_HINT);
    }

    // Load parkour-specific settings
    m_wallRunSensitivity = m_config.GetWallRunSensitivity();
    m_jumpTiming = m_config.GetJumpTiming();
    m_slideControl = m_config.GetSlideControl();
    m_grappleSensitivity = m_config.GetGrappleSensitivity();

    m_difficultyLevel = m_config.GetDifficultyLevel();
    m_timerEnabled = m_config.IsTimerEnabled();
    m_checkpointsEnabled = m_config.AreCheckpointsEnabled();
    m_autoSaveEnabled = m_config.IsAutoSaveEnabled();
    m_speedrunMode = m_config.IsSpeedrunMode();

    m_wallRunEnabled = m_config.IsWallRunEnabled();
    m_doubleJumpEnabled = m_config.IsDoubleJumpEnabled();
    m_slideEnabled = m_config.IsSlideEnabled();
    m_grappleEnabled = m_config.IsGrappleEnabled();
    m_slowMotionOnTrick = m_config.IsSlowMotionOnTrick();
}

void Menu::SaveSettings()
{
    try
    {
        // Save current window settings
        m_config.SetResolution(GetScreenWidth(), GetScreenHeight());
        m_config.SetFullscreen(IsWindowFullscreen());
        m_config.SetVSync(IsWindowState(FLAG_VSYNC_HINT));

        // Save parkour-specific settings
        m_config.SetWallRunSensitivity(m_wallRunSensitivity);
        m_config.SetJumpTiming(m_jumpTiming);
        m_config.SetSlideControl(m_slideControl);
        m_config.SetGrappleSensitivity(m_grappleSensitivity);

        m_config.SetDifficultyLevel(m_difficultyLevel);
        m_config.SetTimerEnabled(m_timerEnabled);
        m_config.SetCheckpointsEnabled(m_checkpointsEnabled);
        m_config.SetAutoSaveEnabled(m_autoSaveEnabled);
        m_config.SetSpeedrunMode(m_speedrunMode);

        m_config.SetWallRunEnabled(m_wallRunEnabled);
        m_config.SetDoubleJumpEnabled(m_doubleJumpEnabled);
        m_config.SetSlideEnabled(m_slideEnabled);
        m_config.SetGrappleEnabled(m_grappleEnabled);
        m_config.SetSlowMotionOnTrick(m_slowMotionOnTrick);

        // Save to file (try current directory first, then build directory)
        if (!m_config.SaveToFile("game.cfg"))
        {
            // Try saving to build directory if not found in current directory
            if (!m_config.SaveToFile("build/game.cfg"))
            {
                TraceLog(LOG_WARNING, "Menu::SaveSettings() - Could not save game.cfg");
            }
            else
            {
                TraceLog(LOG_INFO, "Menu::SaveSettings() - Settings saved to build/game.cfg");
            }
        }
        else
        {
            TraceLog(LOG_INFO, "Menu::SaveSettings() - Settings saved to game.cfg");
        }
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Menu::SaveSettings() - Exception while saving settings: %s", e.what());
    }
}

void Menu::SetAction(MenuAction type)
{
    m_action = type;
}