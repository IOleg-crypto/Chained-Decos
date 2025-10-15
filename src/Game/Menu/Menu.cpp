#include "Menu.h"
#include "Engine/Engine.h"
#include <cstdio> // sscanf
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <string>
#include <algorithm>
#include <vector>
#include <filesystem> // For directory scanning

// Include raylib window functions
// Note: GLFW dependency removed as raylib provides all necessary window management

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

void Menu::HandleKeyboardNavigation()
{
    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        HandleMainMenuKeyboardNavigation();
        break;
    case MenuState::Gameplay:
        HandleGameplayNavigation();
        break;
    case MenuState::Video:
        HandleVideoMenuKeyboardNavigation();
        break;
    case MenuState::Credits:
    case MenuState::Mods:
        HandleSimpleScreenKeyboardNavigation();
        break;
    case MenuState::MapSelection:
        HandleMapSelectionKeyboardNavigation();
        break;
    case MenuState::ConfirmExit:
        HandleConfirmExitKeyboardNavigation();
        break;
    default:
        break;
    }
}

void Menu::HandleVideoNavigation()
{
    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_videoOptions.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_videoOptions.size() - 1) % m_videoOptions.size();

    if (!m_videoOptions[m_selected].values.empty())
    {
        auto &opt = m_videoOptions[m_selected];
        if (IsKeyPressed(KEY_RIGHT))
            opt.selectedIndex = (opt.selectedIndex + 1) % opt.values.size();
        if (IsKeyPressed(KEY_LEFT))
            opt.selectedIndex = (opt.selectedIndex + opt.values.size() - 1) % opt.values.size();
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        auto &opt = m_videoOptions[m_selected];
        if (opt.label == "Back")
            m_state = MenuState::Options;
        else if (opt.label == "Resolution")
        {
            int w = 0, h = 0;
            sscanf(opt.values[opt.selectedIndex].c_str(), "%dx%d", &w, &h);
            m_config.SetResolution(w, h);
            SetWindowSize(w, h);
        }
        else if (opt.label == "Display Mode")
        {
            std::string mode = opt.values[opt.selectedIndex];
            bool fullscreen = (mode == "Fullscreen" || mode == "Borderless");
            m_config.SetFullscreen(fullscreen);

            if (mode == "Fullscreen")
                SetWindowState(FLAG_FULLSCREEN_MODE);
            else if (mode == "Windowed")
            {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
            }
            else if (mode == "Borderless")
                SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_UNDECORATED);
        }
        else if (opt.label == "VSync")
        {
            bool vsync = (opt.values[opt.selectedIndex] == "On");
            m_config.SetVSync(vsync);

            if (vsync)
            {
                SetWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(0);
            }
            else
            {
                ClearWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            }
        }
        else if (opt.label == "Target FPS")
        {
            const std::string val = opt.values[opt.selectedIndex];
            ClearWindowState(FLAG_VSYNC_HINT);

            int fps = (val == "Unlimited") ? 0 : std::stoi(val);
            SetTargetFPS(fps);
        }
    }
}

void Menu::HandleMouseSelection()
{
    Vector2 mousePos = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        HandleMainMenuMouseSelection(mousePos, clicked);
        break;
    case MenuState::Gameplay:
        HandleGameplayMouseSelection(mousePos, clicked);
        break;
    case MenuState::Video:
        HandleVideoMenuMouseSelection(mousePos, clicked);
        break;
    case MenuState::Credits:
        HandleCreditsMouseSelection(mousePos, clicked);
        break;
    case MenuState::Mods:
        HandleModsMouseSelection(mousePos, clicked);
        break;
    case MenuState::MapSelection:
        HandleMapSelectionMouseSelection(mousePos, clicked);
        break;
    case MenuState::ConfirmExit:
        HandleConfirmExitMouseSelection(mousePos, clicked);
        break;
    default:
        break;
    }
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

void Menu::HandleGameplayMouseSelection(Vector2 mousePos, bool clicked)
{
    constexpr int startY = 150, spacing = 80;

    for (size_t i = 0; i < m_gameplayOptions.size(); ++i)
    {
        int y = startY + static_cast<int>(i) * spacing;
        Rectangle rect = {60.0f, (float)(y - 5), (float)(GetScreenWidth() - 120), (float)(spacing - 10)};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                // Ensure m_selected is within bounds before accessing vector
                if (m_selected >= 0 && m_selected < static_cast<int>(m_gameplayOptions.size()))
                {
                    auto &opt = m_gameplayOptions[m_selected];
                    if (opt.label == "Back")
                        m_state = MenuState::Options;
                    else
                        ApplyGameplayOption(opt);
                }
            }
            break;
        }
    }
}


void Menu::HandleConfirmExit()
{
    if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER))
    {
        m_action = MenuAction::ExitGame;
        // ExecuteAction will handle the actual exit
    }
    else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE))
    {
        m_state = MenuState::Main;
        ResetAction();
    }
}

void Menu::HandleMainMenuMouseSelection(Vector2 mousePos, bool clicked)
{
    const std::vector<MenuItem>* menuToUse = GetCurrentMenuWithDynamicItems();
    if (!menuToUse)
        return;

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Scale button dimensions based on screen size
    float scaleFactor = screenWidth / 1920.0f;  // Base on 1920p
    int kBtnW = static_cast<int>(280 * scaleFactor);
    int kBtnH = static_cast<int>(65 * scaleFactor);
    int kStartY = static_cast<int>(320 * scaleFactor);
    int kSpacing = static_cast<int>(85 * scaleFactor);

    // Ensure minimum usable sizes
    if (kBtnW < 200) kBtnW = 200;
    if (kBtnH < 50) kBtnH = 50;
    if (kStartY < 250) kStartY = 250;
    if (kSpacing < 60) kSpacing = 60;

    for (size_t i = 0; i < menuToUse->size(); ++i)
    {
        int x = screenWidth / 2 - kBtnW / 2;
        int y = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle rect = {(float)x, (float)y, (float)kBtnW, (float)kBtnH};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                // Ensure m_selected is within bounds before accessing vector
                if (m_selected >= 0 && m_selected < static_cast<int>(menuToUse->size()))
                {
                    m_action = (*menuToUse)[m_selected].action;
                }
                break;
            }
        }
    }
}

void Menu::HandleVideoMenuMouseSelection(Vector2 mousePos, bool clicked)
{
    constexpr int startY = 150, spacing = 80;

    for (size_t i = 0; i < m_videoOptions.size(); ++i)
    {
        int y = startY + static_cast<int>(i) * spacing;
        Rectangle rect = {60.0f, (float)(y - 5), (float)(GetScreenWidth() - 120), (float)(spacing - 10)};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                auto &opt = m_videoOptions[m_selected];
                if (opt.label == "Back")
                    m_state = MenuState::Options;
                else if (opt.label == "Resolution")
                {
                    int w = 0, h = 0;
                    sscanf(opt.values[opt.selectedIndex].c_str(), "%dx%d", &w, &h);
                    m_config.SetResolution(w, h);
                    SetWindowSize(w, h);
                }
                else if (opt.label == "Display Mode")
                {
                    std::string mode = opt.values[opt.selectedIndex];
                    bool fullscreen = (mode == "Fullscreen" || mode == "Borderless");
                    m_config.SetFullscreen(fullscreen);

                    if (mode == "Fullscreen")
                        SetWindowState(FLAG_FULLSCREEN_MODE);
                    else if (mode == "Windowed")
                    {
                        ClearWindowState(FLAG_FULLSCREEN_MODE);
                        ClearWindowState(FLAG_WINDOW_UNDECORATED);
                    }
                    else if (mode == "Borderless")
                        SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_UNDECORATED);
                }
                else if (opt.label == "VSync")
                {
                    bool vsync = (opt.values[opt.selectedIndex] == "On");
                    m_config.SetVSync(vsync);

                    if (vsync)
                    {
                        SetWindowState(FLAG_VSYNC_HINT);
                        SetTargetFPS(0);
                    }
                    else
                    {
                        ClearWindowState(FLAG_VSYNC_HINT);
                        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
                    }
                }
                else if (opt.label == "Target FPS")
                {
                    const std::string val = opt.values[opt.selectedIndex];
                    ClearWindowState(FLAG_VSYNC_HINT);

                    int fps = (val == "Unlimited") ? 0 : std::stoi(val);
                    SetTargetFPS(fps);
                }
            }
            break;
        }
    }
}

void Menu::HandleCreditsMouseSelection(Vector2 mousePos, bool clicked)
{
    // Credits screen - mouse click anywhere goes back to main menu
    if (clicked)
    {
        m_state = MenuState::Main;
    }
}

void Menu::HandleModsMouseSelection(Vector2 mousePos, bool clicked)
{
    // Mods screen - mouse click anywhere goes back to main menu
    if (clicked)
    {
        m_state = MenuState::Main;
    }
}

void Menu::HandleMapSelectionMouseSelection(Vector2 mousePos, bool clicked)
{
    if (m_availableMaps.empty())
        return;

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    float scaleFactor = screenWidth / 1920.0f;

    // Card-based layout - horizontal arrangement
    const int numCards = 3;
    int cardWidth = static_cast<int>(400 * scaleFactor);
    int cardHeight = static_cast<int>(280 * scaleFactor);
    int spacing = static_cast<int>(60 * scaleFactor);
    int startY = static_cast<int>(140 * scaleFactor);

    // Ensure minimum usable sizes
    if (cardWidth < 300) cardWidth = 300;
    if (cardHeight < 200) cardHeight = 200;
    if (spacing < 40) spacing = 40;
    if (startY < 120) startY = 120;

    int totalWidth = numCards * cardWidth + (numCards - 1) * spacing;
    int startX = (screenWidth - totalWidth) / 2;

    // Check each card for mouse interaction
    for (int i = 0; i < static_cast<int>(m_availableMaps.size()); ++i)
    {
        int x = startX + i * (cardWidth + spacing);
        int y = startY;

        // Apply hover/selection scaling
        bool isSelected = (i == m_selectedMap);
        float cardScale = isSelected ? 1.05f : 1.0f;

        int scaledWidth = static_cast<int>(cardWidth * cardScale);
        int scaledHeight = static_cast<int>(cardHeight * cardScale);
        int cardX = x + (cardWidth - scaledWidth) / 2;
        int cardY = y + (cardHeight - scaledHeight) / 2;

        Rectangle rect = {(float)cardX, (float)cardY, (float)scaledWidth, (float)scaledHeight};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selectedMap = i;
            if (clicked)
            {
                // Start the game with selected map
                const MapInfo* selectedMap = GetSelectedMap();
                if (selectedMap)
                {
                    std::cout << "Starting game with map: " << selectedMap->name << std::endl;
                    // Set action to start game with selected map
                    m_action = MenuAction::StartGameWithMap;
                }
            }
            break;
        }
    }

    // Handle back button click
    float backFontSize = 24.0f * scaleFactor;
    if (backFontSize < 18.0f) backFontSize = 18.0f;
    if (backFontSize > 32.0f) backFontSize = 32.0f;

    const char* backText = "Back";
    int backW = MeasureTextEx(m_font, backText, backFontSize, 2.0f).x;
    int backX = static_cast<int>(30 * scaleFactor);
    if (backX < 20) backX = 20;
    int backY = GetScreenHeight() - static_cast<int>(50 * scaleFactor);
    if (backY > GetScreenHeight() - 40) backY = GetScreenHeight() - 40;

    Rectangle backRect = {(float)backX - 10, (float)backY - 5, (float)backW + 20, (float)backFontSize + 10};

    if (CheckCollisionPointRec(mousePos, backRect) && clicked)
    {
        m_state = MenuState::GameMode;
    }
}

void Menu::HandleConfirmExitMouseSelection(Vector2 mousePos, bool clicked)
{
    if (!clicked)
        return;

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleFactor = screenWidth / 1920.0f;

    int modalWidth = static_cast<int>(500 * scaleFactor);
    int modalHeight = static_cast<int>(300 * scaleFactor);

    // Ensure minimum usable sizes
    if (modalWidth < 400) modalWidth = 400;
    if (modalHeight < 250) modalHeight = 250;
    if (modalWidth > screenWidth - 100) modalWidth = screenWidth - 100;
    if (modalHeight > screenHeight - 100) modalHeight = screenHeight - 100;

    int modalX = screenWidth / 2 - modalWidth / 2;
    int modalY = screenHeight / 2 - modalHeight / 2;

    // YES button (left)
    int yesX = modalX + modalWidth / 2 - static_cast<int>(100 * scaleFactor);
    int buttonY = modalY + modalHeight - static_cast<int>(80 * scaleFactor);
    int buttonWidth = static_cast<int>(80 * scaleFactor);
    int buttonHeight = static_cast<int>(40 * scaleFactor);

    if (buttonWidth < 60) buttonWidth = 60;
    if (buttonHeight < 32) buttonHeight = 32;
    if (yesX < modalX + 20) yesX = modalX + 20;

    Rectangle yesRect = {(float)(yesX - 15), (float)(buttonY - 10), (float)buttonWidth, (float)buttonHeight};

    // NO button (right)
    int noX = modalX + modalWidth / 2 + static_cast<int>(40 * scaleFactor);
    if (noX + buttonWidth + 15 > modalX + modalWidth - 20) noX = modalX + modalWidth - buttonWidth - 35;
    Rectangle noRect = {(float)(noX - 15), (float)(buttonY - 10), (float)buttonWidth, (float)buttonHeight};

    if (CheckCollisionPointRec(mousePos, yesRect))
    {
        m_action = MenuAction::ExitGame;
        // ExecuteAction will handle the actual exit
    }
    else if (CheckCollisionPointRec(mousePos, noRect))
    {
        m_state = MenuState::Main;
        ResetAction();
    }
}


void Menu::HandleMainMenuKeyboardNavigation()
{
    const std::vector<MenuItem>* menuToUse = GetCurrentMenuWithDynamicItems();
    if (!menuToUse)
        return;

    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % menuToUse->size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + menuToUse->size() - 1) % menuToUse->size();

    if (IsKeyPressed(KEY_ENTER))
    {
        // Ensure m_selected is within bounds before accessing vector
        if (m_selected >= 0 && m_selected < static_cast<int>(menuToUse->size()))
        {
            m_action = (*menuToUse)[m_selected].action;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_state == MenuState::Options || m_state == MenuState::GameMode)
            m_state = MenuState::Main;
        else if (m_state == MenuState::Main)
            m_action = MenuAction::ExitGame;
    }
}

void Menu::HandleVideoMenuKeyboardNavigation()
{
    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_videoOptions.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_videoOptions.size() - 1) % m_videoOptions.size();

    if (!m_videoOptions[m_selected].values.empty())
    {
        auto &opt = m_videoOptions[m_selected];
        if (IsKeyPressed(KEY_RIGHT))
            opt.selectedIndex = (opt.selectedIndex + 1) % opt.values.size();
        if (IsKeyPressed(KEY_LEFT))
            opt.selectedIndex = (opt.selectedIndex + opt.values.size() - 1) % opt.values.size();
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        auto &opt = m_videoOptions[m_selected];
        if (opt.label == "Back")
            m_state = MenuState::Options;
        else if (opt.label == "Resolution")
        {
            int w = 0, h = 0;
            sscanf(opt.values[opt.selectedIndex].c_str(), "%dx%d", &w, &h);
            m_config.SetResolution(w, h);
            SetWindowSize(w, h);
        }
        else if (opt.label == "Display Mode")
        {
            std::string mode = opt.values[opt.selectedIndex];
            bool fullscreen = (mode == "Fullscreen" || mode == "Borderless");
            m_config.SetFullscreen(fullscreen);

            if (mode == "Fullscreen")
                SetWindowState(FLAG_FULLSCREEN_MODE);
            else if (mode == "Windowed")
            {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
            }
            else if (mode == "Borderless")
                SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_UNDECORATED);
        }
        else if (opt.label == "VSync")
        {
            bool vsync = (opt.values[opt.selectedIndex] == "On");
            m_config.SetVSync(vsync);

            if (vsync)
            {
                SetWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(0);
            }
            else
            {
                ClearWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            }
        }
        else if (opt.label == "Target FPS")
        {
            const std::string val = opt.values[opt.selectedIndex];
            ClearWindowState(FLAG_VSYNC_HINT);

            int fps = (val == "Unlimited") ? 0 : std::stoi(val);
            SetTargetFPS(fps);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::Options;

    // Handle volume adjustments in Audio menu
    if (m_state == MenuState::Audio)
    {
        const float volumeStep = 0.1f;

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
        {
            float* currentVolume = nullptr;
            std::string volumeName;

            switch (m_selected)
            {
                case 0: // Master Volume
                    currentVolume = &m_masterVolume;
                    volumeName = "Master";
                    break;
                case 1: // Music Volume
                    currentVolume = &m_musicVolume;
                    volumeName = "Music";
                    break;
                case 2: // SFX Volume
                    currentVolume = &m_sfxVolume;
                    volumeName = "SFX";
                    break;
            }

            if (currentVolume)
            {
                if (IsKeyPressed(KEY_RIGHT) && *currentVolume < 1.0f)
                {
                    *currentVolume = std::min(1.0f, *currentVolume + volumeStep);
                    AddConsoleOutput(volumeName + " volume: " + std::to_string(static_cast<int>(*currentVolume * 100)) + "%");
                }
                else if (IsKeyPressed(KEY_LEFT) && *currentVolume > 0.0f)
                {
                    *currentVolume = std::max(0.0f, *currentVolume - volumeStep);
                    AddConsoleOutput(volumeName + " volume: " + std::to_string(static_cast<int>(*currentVolume * 100)) + "%");
                }
            }
        }
    }

    // Handle sensitivity adjustments in Controls menu
    if (m_state == MenuState::Controls && m_selected == 1) // Mouse Sensitivity option
    {
        const float sensitivityStep = 0.1f;

        if (IsKeyPressed(KEY_LEFT) && m_mouseSensitivity > 0.1f)
        {
            m_mouseSensitivity = std::max(0.1f, m_mouseSensitivity - sensitivityStep);
            AddConsoleOutput("Mouse sensitivity: " + std::to_string(m_mouseSensitivity));
        }
        else if (IsKeyPressed(KEY_RIGHT) && m_mouseSensitivity < 3.0f)
        {
            m_mouseSensitivity = std::min(3.0f, m_mouseSensitivity + sensitivityStep);
            AddConsoleOutput("Mouse sensitivity: " + std::to_string(m_mouseSensitivity));
        }
    }

}

void Menu::HandleGameplayNavigation()
{
    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_gameplayOptions.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_gameplayOptions.size() - 1) % m_gameplayOptions.size();

    if (m_selected >= 0 && m_selected < static_cast<int>(m_gameplayOptions.size()))
    {
        if (!m_gameplayOptions[m_selected].values.empty())
        {
            auto &opt = m_gameplayOptions[m_selected];
            if (IsKeyPressed(KEY_RIGHT))
                opt.selectedIndex = (opt.selectedIndex + 1) % opt.values.size();
            if (IsKeyPressed(KEY_LEFT))
                opt.selectedIndex = (opt.selectedIndex + opt.values.size() - 1) % opt.values.size();
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            auto &opt = m_gameplayOptions[m_selected];
            if (opt.label == "Back")
            {
                m_state = MenuState::Options;
            }
            else
            {
                // Apply the selected option immediately
                ApplyGameplayOption(opt);
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::Options;
}



void Menu::ApplyGameplayOption(MenuOption& opt)
{
    if (opt.label == "Difficulty")
    {
        std::string difficulty = opt.values[opt.selectedIndex];
        if (difficulty == "Easy") m_difficultyLevel = 1;
        else if (difficulty == "Medium") m_difficultyLevel = 2;
        else if (difficulty == "Hard") m_difficultyLevel = 3;

        m_config.SetDifficultyLevel(m_difficultyLevel);
        AddConsoleOutput("Difficulty set to: " + difficulty);
    }
    else if (opt.label == "Timer")
    {
        bool enabled = (opt.values[opt.selectedIndex] == "On");
        m_timerEnabled = enabled;
        m_config.SetTimerEnabled(enabled);
        AddConsoleOutput("Timer " + std::string(enabled ? "enabled" : "disabled"));
    }
    else if (opt.label == "Checkpoints")
    {
        bool enabled = (opt.values[opt.selectedIndex] == "On");
        m_checkpointsEnabled = enabled;
        m_config.SetCheckpointsEnabled(enabled);
        AddConsoleOutput("Checkpoints " + std::string(enabled ? "enabled" : "disabled"));
    }
    else if (opt.label == "Auto Save")
    {
        bool enabled = (opt.values[opt.selectedIndex] == "On");
        m_autoSaveEnabled = enabled;
        m_config.SetAutoSaveEnabled(enabled);
        AddConsoleOutput("Auto save " + std::string(enabled ? "enabled" : "disabled"));
    }
    else if (opt.label == "Speedrun Mode")
    {
        bool enabled = (opt.values[opt.selectedIndex] == "On");
        m_speedrunMode = enabled;
        m_config.SetSpeedrunMode(enabled);
        AddConsoleOutput("Speedrun mode " + std::string(enabled ? "enabled" : "disabled"));
    }
}



void Menu::HandleSimpleScreenKeyboardNavigation()
{
    // For Credits and Mods screens - ESC goes back to main menu
    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::Main;
}

void Menu::HandleMapSelectionKeyboardNavigation()
{
    if (m_availableMaps.empty())
        return;

    // Get current page map range
    int startIndex = GetStartMapIndex();
    int endIndex = GetEndMapIndex();
    int mapsOnCurrentPage = endIndex - startIndex;

    // Horizontal navigation within current page (left/right only)
    if (IsKeyPressed(KEY_LEFT))
    {
        if (m_selectedMap > startIndex)
            m_selectedMap--;
        else if (m_currentPage > 0)
        {
            // Move to previous page and select last map on that page
            PreviousPage();
            UpdatePagination();
            startIndex = GetStartMapIndex();
            m_selectedMap = std::min(startIndex + mapsOnCurrentPage - 1, static_cast<int>(m_availableMaps.size()) - 1);
        }
        else
        {
            // Wrap to last page and select last map
            m_currentPage = m_totalPages - 1;
            UpdatePagination();
            startIndex = GetStartMapIndex();
            m_selectedMap = std::min(startIndex + mapsOnCurrentPage - 1, static_cast<int>(m_availableMaps.size()) - 1);
        }

        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Map selection changed to: %d (Page %d/%d)",
                 m_selectedMap, m_currentPage + 1, m_totalPages);
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        if (m_selectedMap < endIndex - 1)
            m_selectedMap++;
        else if (m_currentPage < m_totalPages - 1)
        {
            // Move to next page and select first map on that page
            NextPage();
            UpdatePagination();
            m_selectedMap = GetStartMapIndex();
        }
        else
        {
            // Wrap to first page and select first map
            m_currentPage = 0;
            UpdatePagination();
            m_selectedMap = 0;
        }

        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Map selection changed to: %d (Page %d/%d)",
                 m_selectedMap, m_currentPage + 1, m_totalPages);
    }

    // Page navigation
    if (IsKeyPressed(KEY_PAGE_DOWN) || (IsKeyPressed(KEY_RIGHT) && IsKeyDown(KEY_LEFT_CONTROL)))
    {
        NextPage();
        UpdatePagination();
        m_selectedMap = GetStartMapIndex();
        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Next page: %d/%d, selected map: %d",
                 m_currentPage + 1, m_totalPages, m_selectedMap);
    }

    if (IsKeyPressed(KEY_PAGE_UP) || (IsKeyPressed(KEY_LEFT) && IsKeyDown(KEY_LEFT_CONTROL)))
    {
        PreviousPage();
        UpdatePagination();
        m_selectedMap = GetStartMapIndex();
        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Previous page: %d/%d, selected map: %d",
                 m_currentPage + 1, m_totalPages, m_selectedMap);
    }

    if (IsKeyPressed(KEY_ENTER) && !m_availableMaps.empty())
    {
        // Start the game with selected map
        const MapInfo* selectedMap = GetSelectedMap();
        if (selectedMap)
        {
            TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Starting game with map: %s", selectedMap->displayName.c_str());
            // Set action to start game with selected map
            m_action = MenuAction::StartGameWithMap;
        }
        else
        {
            TraceLog(LOG_ERROR, "Menu::HandleMapSelectionKeyboardNavigation() - No map selected!");
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Returning to game mode menu");
        m_state = MenuState::GameMode;
    }
}

void Menu::HandleConfirmExitKeyboardNavigation()
{
    if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER))
    {
        m_action = MenuAction::ExitGame;
        if (m_engine)
            m_engine->RequestExit();
    }
    else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE))
    {
        m_state = MenuState::Main;
    }
}

void Menu::Render() {
    // Dark theme background matching the provided design
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{23, 23, 23, 255});

    // Subtle gradient overlay
    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = (float)i / GetScreenHeight();
        Color gradientColor = Color{
            (unsigned char)(23 + 10 * t),
            (unsigned char)(23 + 10 * t),
            (unsigned char)(23 + 15 * t),
            255
        };
        DrawLine(0, i, GetScreenWidth(), i, gradientColor);
    }

    // Add subtle animated overlay pattern
    static float time = 0.0f;
    time += GetFrameTime();

    for (int i = 0; i < 15; i++)
    {
        float y = (GetScreenHeight() / 15) * i + sinf(time + i * 0.3f) * 5;
        DrawLine(0, (int)y, GetScreenWidth(), (int)y,
                 Fade(Color{60, 80, 120, 255}, 0.05f + sinf(time * 1.5f + i) * 0.03f));
    }

    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
        RenderMenu();
        break;
    case MenuState::Audio:
    case MenuState::Controls:
        RenderMenu();
        break;
    case MenuState::Gameplay:
        RenderGameplayMenu();
        break;
    case MenuState::Video:
        RenderSettingsMenu();
        break;
    case MenuState::Credits:
        RenderCredits();
        break;
    case MenuState::Mods:
        RenderMods();
        break;
    case MenuState::MapSelection:
        RenderMapSelection();
        break;
    case MenuState::ConfirmExit:
        RenderConfirmExit();
        break;
    default:
        break;
    }

    // Render console on top of everything
    RenderConsole();
}

void Menu::RenderMenu()
{
    if (!m_currentMenu)
        return;

    // Dark theme background matching the provided design
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{23, 23, 23, 255});

    // Subtle gradient overlay
    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = (float)i / GetScreenHeight();
        Color gradientColor = Color{
            (unsigned char)(23 + 10 * t),
            (unsigned char)(23 + 10 * t),
            (unsigned char)(23 + 15 * t),
            255
        };
        DrawLine(0, i, GetScreenWidth(), i, gradientColor);
    }

    Vector2 mousePos = GetMousePosition();

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleFactor = screenWidth / 1920.0f;  // Base on 1920p

    int kBtnW = static_cast<int>(280 * scaleFactor);
    int kBtnH = static_cast<int>(65 * scaleFactor);
    int kStartY = static_cast<int>(320 * scaleFactor);
    int kSpacing = static_cast<int>(85 * scaleFactor);

    // Ensure minimum usable sizes
    if (kBtnW < 200) kBtnW = 200;
    if (kBtnH < 50) kBtnH = 50;
    if (kStartY < 250) kStartY = 250;
    if (kSpacing < 60) kSpacing = 60;

    // Get current menu with dynamic items
    const std::vector<MenuItem>* menuToRender = GetCurrentMenuWithDynamicItems();
    if (!menuToRender)
        return;

    // Modern title with glow effect
    const char *title = (m_state == MenuState::Main) ? "CHAINED DECOS" :
                       (m_selected >= 0 && m_selected < static_cast<int>(menuToRender->size())) ?
                       (*menuToRender)[m_selected].label : "UNKNOWN";

    // Dynamic title sizing
    float titleFontSize = 60.0f * scaleFactor;
    if (titleFontSize < 40.0f) titleFontSize = 40.0f;
    if (titleFontSize > 80.0f) titleFontSize = 80.0f;

    // Draw title glow/shadow
    int tw = MeasureTextEx(m_font, title, titleFontSize, 2.0f).x;
    int titleX = screenWidth / 2 - tw / 2;
    int titleY = static_cast<int>(80 * scaleFactor);
    if (titleY < 60) titleY = 60;

    // Multiple glow layers for depth
    for (int i = 3; i >= 1; i--)
    {
        DrawTextEx(m_font, title, Vector2{(float)titleX + i, (float)titleY + i}, titleFontSize, 2.0f, Fade(Color{100, 150, 255, 255}, 0.3f / i));
    }

    // Main title with modern color
    DrawTextEx(m_font, title, Vector2{(float)titleX, (float)titleY}, titleFontSize, 2.0f, Color{220, 220, 220, 255});

    // Subtitle for main menu
    if (m_state == MenuState::Main)
    {
        const char *subtitle = "Modern 3D Platformer";
        float subtitleFontSize = 24.0f * scaleFactor;
        if (subtitleFontSize < 18.0f) subtitleFontSize = 18.0f;
        if (subtitleFontSize > 36.0f) subtitleFontSize = 36.0f;

        int stw = MeasureTextEx(m_font, subtitle, subtitleFontSize, 2.0f).x;
        int subtitleY = titleY + static_cast<int>(50 * scaleFactor);
        if (subtitleY < titleY + 40) subtitleY = titleY + 40;

        DrawTextEx(m_font, subtitle, Vector2{(float)(screenWidth / 2 - stw / 2), (float)subtitleY}, subtitleFontSize, 2.0f, Fade(Color{150, 180, 220, 255}, 0.8f));

        // Version info in bottom corner
        float versionFontSize = 16.0f * scaleFactor;
        if (versionFontSize < 12.0f) versionFontSize = 12.0f;
        if (versionFontSize > 24.0f) versionFontSize = 24.0f;

        const char *version = "v1.0.0";
        int vw = MeasureTextEx(m_font, version, versionFontSize, 2.0f).x;
        int versionY = screenHeight - static_cast<int>(25 * scaleFactor);
        if (versionY > screenHeight - 20) versionY = screenHeight - 20;

        DrawTextEx(m_font, version, Vector2{(float)(screenWidth - vw - 20), (float)versionY}, versionFontSize, 2.0f, Fade(Color{120, 140, 160, 255}, 0.7f));
    }

    if (m_buttonScales.size() != menuToRender->size())
    {
        m_buttonScales.assign(menuToRender->size(), 1.0f);
        // Ensure m_selected is within bounds after resizing
        if (m_selected >= static_cast<int>(menuToRender->size()))
            m_selected = static_cast<int>(menuToRender->size()) - 1;
        if (m_selected < 0)
            m_selected = 0;
    }

    for (size_t i = 0; i < menuToRender->size(); ++i)
    {
        const auto &item = (i < menuToRender->size()) ? (*menuToRender)[i] : (*menuToRender)[0];
        int baseX = GetScreenWidth() / 2 - kBtnW / 2;
        int baseY = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle rect = {(float)baseX, (float)baseY, (float)kBtnW, (float)kBtnH};

        bool hovered = CheckCollisionPointRec(mousePos, rect);
        bool selected = (m_selected >= 0 && m_selected < static_cast<int>(menuToRender->size()) && static_cast<int>(i) == m_selected);
        float targetScale = (hovered || selected) ? 1.05f : 1.0f;
        if (i < m_buttonScales.size())
        {
            m_buttonScales[i] = Lerp(m_buttonScales[i], targetScale, 0.2f);
        }

        int w = static_cast<int>(kBtnW * m_buttonScales[i]);
        int h = static_cast<int>(kBtnH * m_buttonScales[i]);
        int x = GetScreenWidth() / 2 - w / 2;
        int y = baseY - (h - kBtnH) / 2;
        Rectangle btnRect = {(float)x, (float)y, (float)w, (float)h};

        // Dark theme button design
        Color baseColor, accentColor, glowColor;

        if (selected)
        {
            baseColor = {80, 100, 140, 255};     // Dark blue selection
            accentColor = {120, 150, 200, 255};  // Light blue accent
            glowColor = {150, 180, 255, 150};    // Blue glow
        }
        else if (hovered)
        {
            baseColor = {60, 70, 90, 255};       // Dark gray-blue hover
            accentColor = {90, 110, 140, 255};   // Medium blue accent
            glowColor = {120, 140, 180, 100};    // Subtle blue glow
        }
        else
        {
            baseColor = {45, 50, 60, 255};       // Dark neutral
            accentColor = {70, 80, 100, 255};    // Darker accent
            glowColor = {0, 0, 0, 0};           // No glow
        }

        // Draw glow effect for selected/hovered buttons
        if (selected || hovered)
        {
            for (int g = 6; g >= 2; g -= 2)
            {
                float alpha = (selected) ? 0.3f / (g/2) : 0.15f / (g/2);
                DrawRectangle(x - g, y - g, w + g*2, h + g*2,
                             Fade(glowColor, alpha));
            }
        }

        // Main button background with subtle gradient
        for (int j = 0; j < h; ++j)
        {
            float t = (float)j / h;
            float intensity = 1.0f - t * 0.2f; // Slightly darker at bottom
            Color c = {
                (unsigned char)(baseColor.r * intensity),
                (unsigned char)(baseColor.g * intensity),
                (unsigned char)(baseColor.b * intensity),
                baseColor.a
            };
            DrawLine(x, y + j, x + w, y + j, c);
        }

        // Subtle highlight/shine effect
        DrawRectangle(x + 2, y + 2, w - 4, h/4, Fade(Color{255, 255, 255, 255}, 0.1f));

        // Modern border
        if (selected)
        {
            DrawRectangleLinesEx(btnRect, 2, accentColor);
            // Double border for selected
            DrawRectangleLinesEx(Rectangle{btnRect.x - 1, btnRect.y - 1, btnRect.width + 2, btnRect.height + 2}, 1, Fade(accentColor, 0.6f));
        }
        else if (hovered)
        {
            DrawRectangleLinesEx(btnRect, 1, accentColor);
        }
        else
        {
            DrawRectangleLinesEx(btnRect, 1, Color{60, 70, 80, 255});
        }

        // Modern text with better font and effects - dynamic sizing
        float baseTextSize = selected ? 30.0f : (hovered ? 28.0f : 26.0f);
        float textSize = baseTextSize * scaleFactor;
        if (textSize < 20.0f) textSize = 20.0f;
        if (textSize > 40.0f) textSize = 40.0f;

        Color textColor = selected ? Color{220, 230, 255, 255} : (hovered ? Color{200, 210, 230, 255} : Color{180, 190, 210, 255});

        int textW = MeasureTextEx(m_font, (i < menuToRender->size()) ? item.label : "UNKNOWN", textSize, 2.0f).x;
        int textX = x + w / 2 - textW / 2;
        int textY = y + h / 2 - static_cast<int>(textSize / 2);

        // Text shadow for depth
        DrawTextEx(m_font, (i < menuToRender->size()) ? item.label : "UNKNOWN", Vector2{(float)textX + 1, (float)textY + 1}, textSize, 2.0f, Fade(Color{0, 0, 0, 255}, 0.6f));

        // Main text with modern color
        DrawTextEx(m_font, (i < menuToRender->size()) ? item.label : "UNKNOWN", Vector2{(float)textX, (float)textY}, textSize, 2.0f, textColor);
    }

    // Modern footer with dark theme styling - dynamic sizing
    float footerFontSize = 18.0f * scaleFactor;
    if (footerFontSize < 14.0f) footerFontSize = 14.0f;
    if (footerFontSize > 26.0f) footerFontSize = 26.0f;

    const char *footer = "ENTER Select    ESC Back    ↑↓ Navigate    MOUSE Click";
    int fw = MeasureTextEx(m_font, footer, footerFontSize, 2.0f).x;
    int footerX = screenWidth / 2 - fw / 2;
    int footerY = screenHeight - static_cast<int>(35 * scaleFactor);
    if (footerY > screenHeight - 25) footerY = screenHeight - 25;

    // Footer background
    int footerHeight = static_cast<int>(28 * scaleFactor);
    if (footerHeight < 22) footerHeight = 22;
    DrawRectangle(footerX - 10, footerY - 5, fw + 20, footerHeight, Fade(Color{0, 0, 0, 255}, 0.5f));
    DrawRectangleLines(footerX - 10, footerY - 5, fw + 20, footerHeight, Fade(Color{80, 90, 100, 255}, 0.6f));

    // Modern footer text with color coding - dynamic positioning
    float enterWidth = MeasureTextEx(m_font, "ENTER", footerFontSize, 2.0f).x;
    float selectWidth = MeasureTextEx(m_font, " Select    ", footerFontSize, 2.0f).x;
    float escWidth = MeasureTextEx(m_font, "ESC", footerFontSize, 2.0f).x;
    float backWidth = MeasureTextEx(m_font, " Back    ", footerFontSize, 2.0f).x;
    float navWidth = MeasureTextEx(m_font, "↑↓", footerFontSize, 2.0f).x;
    float navigateWidth = MeasureTextEx(m_font, " Navigate    ", footerFontSize, 2.0f).x;
    float mouseWidth = MeasureTextEx(m_font, "MOUSE", footerFontSize, 2.0f).x;
    float clickWidth = MeasureTextEx(m_font, " Click", footerFontSize, 2.0f).x;

    DrawTextEx(m_font, "ENTER", Vector2{(float)footerX, (float)footerY}, footerFontSize, 2.0f, Color{100, 200, 120, 255});
    DrawTextEx(m_font, " Select    ", Vector2{(float)footerX + enterWidth, (float)footerY}, footerFontSize, 2.0f, Color{180, 190, 200, 255});
    DrawTextEx(m_font, "ESC", Vector2{(float)footerX + enterWidth + selectWidth, (float)footerY}, footerFontSize, 2.0f, Color{200, 100, 100, 255});
    DrawTextEx(m_font, " Back    ", Vector2{(float)footerX + enterWidth + selectWidth + escWidth, (float)footerY}, footerFontSize, 2.0f, Color{180, 190, 200, 255});
    DrawTextEx(m_font, "↑↓", Vector2{(float)footerX + enterWidth + selectWidth + escWidth + backWidth, (float)footerY}, footerFontSize, 2.0f, Color{120, 150, 200, 255});
    DrawTextEx(m_font, " Navigate    ", Vector2{(float)footerX + enterWidth + selectWidth + escWidth + backWidth + navWidth, (float)footerY}, footerFontSize, 2.0f, Color{180, 190, 200, 255});
    DrawTextEx(m_font, "MOUSE", Vector2{(float)footerX + enterWidth + selectWidth + escWidth + backWidth + navWidth + navigateWidth, (float)footerY}, footerFontSize, 2.0f, Color{200, 180, 100, 255});
    DrawTextEx(m_font, " Click", Vector2{(float)footerX + enterWidth + selectWidth + escWidth + backWidth + navWidth + navigateWidth + mouseWidth, (float)footerY}, footerFontSize, 2.0f, Color{180, 190, 200, 255});
}

void Menu::RenderSettingsMenu() const {
    int startY = 150, spacing = 80, fontSize = 30;

    // Modern settings title with glow
    const char* settingsTitle = "SETTINGS";
    int titleW = MeasureTextEx(m_font, settingsTitle, 45, 2.0f).x;
    int titleX = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX + i, (float)45 + i}, 45, 2.0f, Fade(Color{255, 150, 50, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX, (float)45}, 45, 2.0f, Color{255, 200, 100, 255});

    for (size_t i = 0; i < m_videoOptions.size(); ++i)
    {
        auto &opt = m_videoOptions[i];
        int y = startY + static_cast<int>(i) * spacing;

        bool isSelected = (static_cast<int>(i) == m_selected);
        Color labelColor = isSelected ? Color{255, 220, 150, 255} : Color{200, 210, 230, 255};

        // Modern setting container
        if (isSelected)
        {
            // Background highlight for selected option
            DrawRectangle(60, y - 5, GetScreenWidth() - 120, spacing - 10, Fade(Color{100, 150, 255, 255}, 0.2f));
            DrawRectangleLines(60, y - 5, GetScreenWidth() - 120, spacing - 10, Color{150, 200, 255, 255});
        }

        // Draw setting label with modern typography
        const char* label = opt.label.c_str();
        int labelW = MeasureTextEx(m_font, label, fontSize, 2.0f).x;
        DrawTextEx(m_font, label, Vector2{80.0f, (float)y + 5}, fontSize, 2.0f, labelColor);

        if (!opt.values.empty())
        {
            // Get current system value for display
            std::string currentValue = GetCurrentSettingValue(opt.label);
            if (!currentValue.empty())
            {
                int currentWidth = MeasureTextEx(m_font, currentValue.c_str(), fontSize - 8, 2.0f).x;
                DrawTextEx(m_font, currentValue.c_str(), Vector2{(float)80 + 320, (float)y + 8}, fontSize - 8, 2.0f,
                          isSelected ? Fade(Color{255, 255, 150, 255}, 0.9f) : Fade(Color{180, 200, 150, 255}, 0.7f));
            }

            // Show selected value with modern styling
            std::string displayValue;
            if (opt.selectedIndex < opt.values.size())
            {
                displayValue = opt.values[opt.selectedIndex];
            }

            if (!displayValue.empty())
            {
                // Modern value display with background
                int textWidth = MeasureTextEx(m_font, displayValue.c_str(), fontSize, 2.0f).x;
                int xPos = GetScreenWidth() - textWidth - 100;

                if (isSelected)
                {
                    // Background for selected value
                    DrawRectangle(xPos - 10, y - 2, textWidth + 20, fontSize + 8, Fade(Color{255, 200, 100, 255}, 0.3f));
                    DrawRectangleLines(xPos - 10, y - 2, textWidth + 20, fontSize + 8, Color{255, 180, 80, 255});
                }

                DrawTextEx(m_font, displayValue.c_str(), Vector2{(float)xPos, (float)y + 5}, fontSize, 2.0f,
                          isSelected ? Color{255, 255, 180, 255} : Color{220, 230, 200, 255});
            }
        }
    }

    // Modern settings footer
    std::string footer = "ENTER Apply/Select    Arrow L/R Change    ↑↓ Navigate    ESC Back";
    int fw = MeasureTextEx(m_font, footer.c_str(), 18, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw / 2;
    int footerY = GetScreenHeight() - 30;

    // Footer background
    DrawRectangle(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    // Color-coded footer text
    DrawTextEx(m_font, "ENTER", Vector2{(float)footerX, (float)footerY}, 18, 2.0f, Color{150, 255, 150, 255});
    DrawTextEx(m_font, " Apply/Select    ", Vector2{(float)footerX + 65, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
    DrawTextEx(m_font, "←→", Vector2{(float)footerX + 190, (float)footerY}, 18, 2.0f, Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Change    ", Vector2{(float)footerX + 210, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
    DrawTextEx(m_font, "↑↓", Vector2{(float)footerX + 290, (float)footerY}, 18, 2.0f, Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Navigate    ", Vector2{(float)footerX + 310, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
    DrawTextEx(m_font, "ESC", Vector2{(float)footerX + 410, (float)footerY}, 18, 2.0f, Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 440, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
}

void Menu::RenderCredits()
{
    // Modern credits title with glow
    const char *title = "CREDITS";
    int tw = MeasureTextEx(m_font, title, 50, 2.0f).x;
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, title, Vector2{(float)titleX + i, (float)titleY + i}, 50, 2.0f, Fade(Color{255, 150, 100, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, title, Vector2{(float)titleX, (float)titleY}, 50, 2.0f, Color{255, 200, 150, 255});

    // Modern credits content with better layout
    int y = 180, fs = 28;

    // Developer credit with modern styling
    DrawTextEx(m_font, "DEVELOPER", Vector2{80.0f, (float)y}, 24, 2.0f, Color{200, 220, 255, 255});
    DrawTextEx(m_font, "I#Oleg", Vector2{80.0f, (float)y + 30}, fs, 2.0f, Color{255, 255, 200, 255});

    y += 100;
    DrawTextEx(m_font, "ENGINE", Vector2{80.0f, (float)y}, 24, 2.0f, Color{200, 220, 255, 255});
    DrawTextEx(m_font, "raylib + rlImGui", Vector2{80.0f, (float)y + 30}, fs, 2.0f, Color{255, 255, 200, 255});

    y += 100;
    DrawTextEx(m_font, "UI DESIGN", Vector2{80.0f, (float)y}, 24, 2.0f, Color{200, 220, 255, 255});
    DrawTextEx(m_font, "Modern Interface", Vector2{80.0f, (float)y + 30}, fs, 2.0f, Color{255, 255, 200, 255});

    // Modern footer
    const char *footer = "ESC Back";
    int fw2 = MeasureTextEx(m_font, footer, 20, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw2 / 2;
    int footerY = GetScreenHeight() - 30;

    DrawRectangle(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    DrawTextEx(m_font, "ESC", Vector2{(float)footerX, (float)footerY}, 20, 2.0f, Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 35, (float)footerY}, 20, 2.0f, Color{200, 200, 200, 255});
}

void Menu::RenderMods()
{
    // Modern mods title with glow
    const char *title = "MODS";
    int tw = MeasureTextEx(m_font, title, 50, 2.0f).x;
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, title, Vector2{(float)titleX + i, (float)titleY + i}, 50, 2.0f, Fade(Color{200, 100, 255, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, title, Vector2{(float)titleX, (float)titleY}, 50, 2.0f, Color{220, 150, 255, 255});

    // Modern content layout
    int y = 180, fs = 26;

    // No mods message with modern styling
    const char* noModsMsg = "NO MODS DETECTED";
    int noModsW = MeasureTextEx(m_font, noModsMsg, 28, 2.0f).x;
    int noModsX = GetScreenWidth() / 2 - noModsW / 2;
    DrawTextEx(m_font, noModsMsg, Vector2{(float)noModsX, (float)y}, 28, 2.0f, Color{255, 200, 150, 255});

    y += 80;
    const char* instructionMsg = "Place your mods in the 'resources/mods' folder";
    int instructionW = MeasureTextEx(m_font, instructionMsg, fs, 2.0f).x;
    int instructionX = GetScreenWidth() / 2 - instructionW / 2;
    DrawTextEx(m_font, instructionMsg, Vector2{(float)instructionX, (float)y}, fs, 2.0f, Color{180, 200, 220, 255});

    // Modern footer
    const char *footer = "ESC Back";
    int fw2 = MeasureTextEx(m_font, footer, 20, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw2 / 2;
    int footerY = GetScreenHeight() - 30;

    DrawRectangle(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    DrawTextEx(m_font, "ESC", Vector2{(float)footerX, (float)footerY}, 20, 2.0f, Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 35, (float)footerY}, 20, 2.0f, Color{200, 200, 200, 255});
}

void Menu::RenderGameplayMenu()
{
    int startY = 150, spacing = 80, fontSize = 30;

    // Modern gameplay settings title with glow
    const char* settingsTitle = "GAMEPLAY SETTINGS";
    int titleW = MeasureTextEx(m_font, settingsTitle, 45, 2.0f).x;
    int titleX = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX + i, (float)45 + i}, 45, 2.0f, Fade(Color{100, 255, 150, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX, (float)45}, 45, 2.0f, Color{150, 255, 200, 255});

    for (size_t i = 0; i < m_gameplayOptions.size(); ++i)
    {
        auto &opt = m_gameplayOptions[i];
        int y = startY + static_cast<int>(i) * spacing;

        bool isSelected = (static_cast<int>(i) == m_selected);
        Color labelColor = isSelected ? Color{255, 220, 150, 255} : Color{200, 210, 230, 255};

        // Modern setting container
        if (isSelected)
        {
            // Background highlight for selected option
            DrawRectangle(60, y - 5, GetScreenWidth() - 120, spacing - 10, Fade(Color{100, 255, 150, 255}, 0.2f));
            DrawRectangleLines(60, y - 5, GetScreenWidth() - 120, spacing - 10, Color{150, 255, 200, 255});
        }

        // Draw setting label with modern typography
        const char* label = opt.label.c_str();
        int labelW = MeasureTextEx(m_font, label, fontSize, 2.0f).x;
        DrawTextEx(m_font, label, Vector2{80.0f, (float)y + 5}, fontSize, 2.0f, labelColor);

        if (!opt.values.empty())
        {
            // Show current value (smaller font, different position)
            std::string currentValue = GetGameplaySettingValue(opt.label);
            if (!currentValue.empty())
            {
                int currentWidth = MeasureTextEx(m_font, currentValue.c_str(), fontSize - 8, 2.0f).x;
                DrawTextEx(m_font, currentValue.c_str(), Vector2{(float)80 + 320, (float)y + 8}, fontSize - 8, 2.0f,
                          isSelected ? Fade(Color{255, 255, 150, 255}, 0.9f) : Fade(Color{180, 200, 150, 255}, 0.7f));
            }

            // Show selected value with modern styling
            std::string displayValue;
            if (opt.selectedIndex >= 0 && opt.selectedIndex < static_cast<int>(opt.values.size()))
            {
                displayValue = opt.values[opt.selectedIndex];
            }

            if (!displayValue.empty())
            {
                // Modern value display with background
                int textWidth = MeasureTextEx(m_font, displayValue.c_str(), fontSize, 2.0f).x;
                int xPos = GetScreenWidth() - textWidth - 100;

                if (isSelected)
                {
                    // Background for selected value
                    DrawRectangle(xPos - 10, y - 2, textWidth + 20, fontSize + 8, Fade(Color{255, 200, 100, 255}, 0.3f));
                    DrawRectangleLines(xPos - 10, y - 2, textWidth + 20, fontSize + 8, Color{255, 180, 80, 255});
                }

                DrawTextEx(m_font, displayValue.c_str(), Vector2{(float)xPos, (float)y + 5}, fontSize, 2.0f,
                          isSelected ? Color{255, 255, 180, 255} : Color{220, 230, 200, 255});
            }
        }
    }

    // Modern settings footer
    std::string footer = "ENTER Apply/Select    ←→ Change    ↑↓ Navigate    ESC Back";
    int fw = MeasureTextEx(m_font, footer.c_str(), 18, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw / 2;
    int footerY = GetScreenHeight() - 30;

    // Footer background
    DrawRectangle(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    // Color-coded footer text
    DrawTextEx(m_font, "ENTER", Vector2{(float)footerX, (float)footerY}, 18, 2.0f, Color{150, 255, 150, 255});
    DrawTextEx(m_font, " Apply/Select    ", Vector2{(float)footerX + 65, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
    DrawTextEx(m_font, "←→", Vector2{(float)footerX + 190, (float)footerY}, 18, 2.0f, Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Change    ", Vector2{(float)footerX + 210, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
    DrawTextEx(m_font, "↑↓", Vector2{(float)footerX + 290, (float)footerY}, 18, 2.0f, Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Navigate    ", Vector2{(float)footerX + 310, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
    DrawTextEx(m_font, "ESC", Vector2{(float)footerX + 410, (float)footerY}, 18, 2.0f, Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 440, (float)footerY}, 18, 2.0f, Color{200, 200, 200, 255});
}


std::string Menu::GetGameplaySettingValue(const std::string& settingName) const
{
   if (settingName == "Difficulty")
   {
       switch (m_difficultyLevel)
       {
           case 1: return "Easy";
           case 2: return "Medium";
           case 3: return "Hard";
           default: return "Medium";
       }
   }
   else if (settingName == "Timer")
   {
       return m_timerEnabled ? "On" : "Off";
   }
   else if (settingName == "Checkpoints")
   {
       return m_checkpointsEnabled ? "On" : "Off";
   }
   else if (settingName == "Auto Save")
   {
       return m_autoSaveEnabled ? "On" : "Off";
   }
   else if (settingName == "Speedrun Mode")
   {
       return m_speedrunMode ? "On" : "Off";
   }

   return "";
}


std::string Menu::GetCurrentSettingValue(const std::string& settingName) const
{
   if (settingName == "Resolution")
   {
       int width = GetScreenWidth();
       int height = GetScreenHeight();
       return TextFormat("%dx%d", width, height);
   }
   else if (settingName == "Display Mode")
   {
       // Check if we're in fullscreen mode
       bool isFullscreen = IsWindowFullscreen();
       if (isFullscreen)
       {
           // Check if window is decorated (not borderless)
           bool isDecorated = !IsWindowState(FLAG_WINDOW_UNDECORATED);
           if (isDecorated)
               return "Fullscreen";
           else
               return "Borderless";
       }
       else
           return "Windowed";
   }
   else if (settingName == "VSync")
   {
       return (FLAG_VSYNC_HINT) ? "On" : "Off";
   }
   else if (settingName == "Target FPS")
   {
       int targetFPS = GetFPS();
       if (targetFPS == 0)
           return "Unlimited";
       else
           return TextFormat("%d", targetFPS);
   }
   else if (settingName == "Aspect Ratio")
   {
       int width = GetScreenWidth();
       int height = GetScreenHeight();
       float aspect = (float)width / (float)height;

       if (fabsf(aspect - 16.0f/9.0f) < 0.1f)
           return "16:9";
       else if (fabsf(aspect - 4.0f/3.0f) < 0.1f)
           return "4:3";
       else if (fabsf(aspect - 21.0f/9.0f) < 0.1f)
           return "21:9";
       else
           return TextFormat("%.2f:1", aspect);
   }
   else if (settingName == "Master Volume")
   {
       return TextFormat("%.0f%%", m_masterVolume * 100);
   }
   else if (settingName == "Music Volume")
   {
       return TextFormat("%.0f%%", m_musicVolume * 100);
   }
   else if (settingName == "SFX Volume")
   {
       return TextFormat("%.0f%%", m_sfxVolume * 100);
   }
   else if (settingName == "Mouse Sensitivity")
   {
       return TextFormat("%.1fx", m_mouseSensitivity);
   }
   else if (settingName == "Invert Y Axis")
   {
       return m_invertYAxis ? "On" : "Off";
   }
   else if (settingName == "Controller Support")
   {
       return m_controllerSupport ? "On" : "Off";
   }

   return "";
}

void Menu::RenderConfirmExit()
{
    // Modern modal background with blur effect
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Color{0, 0, 0, 180}, 0.7f));

    // Modal dialog container - dynamic sizing
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleFactor = screenWidth / 1920.0f;

    int modalWidth = static_cast<int>(500 * scaleFactor);
    int modalHeight = static_cast<int>(300 * scaleFactor);

    // Ensure minimum usable sizes
    if (modalWidth < 400) modalWidth = 400;
    if (modalHeight < 250) modalHeight = 250;
    if (modalWidth > screenWidth - 100) modalWidth = screenWidth - 100;
    if (modalHeight > screenHeight - 100) modalHeight = screenHeight - 100;

    int modalX = screenWidth / 2 - modalWidth / 2;
    int modalY = screenHeight / 2 - modalHeight / 2;

    // Modal background with gradient
    for (int i = 0; i < modalHeight; i++)
    {
        float t = (float)i / modalHeight;
        Color c = {
            (unsigned char)(60 + 40 * t),
            (unsigned char)(40 + 30 * t),
            (unsigned char)(80 + 60 * t),
            220
        };
        DrawLine(modalX, modalY + i, modalX + modalWidth, modalY + i, c);
    }

    // Modal border with glow
    for (int g = 3; g >= 1; g--)
    {
        DrawRectangleLines(modalX - g, modalY - g, modalWidth + g*2, modalHeight + g*2,
                          Fade(Color{150, 100, 255, 255}, 0.3f / g));
    }

    // Modern title - dynamic sizing
    float titleFontSize = 40.0f * scaleFactor;
    if (titleFontSize < 30.0f) titleFontSize = 30.0f;
    if (titleFontSize > 60.0f) titleFontSize = 60.0f;

    const char *msg = "EXIT GAME?";
    int tw = MeasureTextEx(m_font, msg, titleFontSize, 2.0f).x;
    int titleX = modalX + modalWidth / 2 - tw / 2;
    int titleY = modalY + static_cast<int>(40 * scaleFactor);
    if (titleY < modalY + 30) titleY = modalY + 30;

    // Title glow
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, msg, Vector2{(float)titleX + i, (float)titleY + i}, titleFontSize, 2.0f, Fade(Color{255, 100, 100, 255}, 0.6f / i));
    }
    DrawTextEx(m_font, msg, Vector2{(float)titleX, (float)titleY}, titleFontSize, 2.0f, Color{255, 150, 150, 255});

    // Modern buttons with better styling - dynamic sizing
    float buttonFontSize = 28.0f * scaleFactor;
    if (buttonFontSize < 22.0f) buttonFontSize = 22.0f;
    if (buttonFontSize > 40.0f) buttonFontSize = 40.0f;

    const char *yes = "YES";
    const char *no = "NO";
    int yw = MeasureTextEx(m_font, yes, buttonFontSize, 2.0f).x;
    int nw = MeasureTextEx(m_font, no, buttonFontSize, 2.0f).x;

    int buttonY = modalY + modalHeight - static_cast<int>(80 * scaleFactor);
    if (buttonY > modalY + modalHeight - 60) buttonY = modalY + modalHeight - 60;

    int buttonHeight = static_cast<int>(40 * scaleFactor);
    if (buttonHeight < 32) buttonHeight = 32;

    // YES button (left)
    int yesX = modalX + modalWidth / 2 - yw - static_cast<int>(40 * scaleFactor);
    if (yesX < modalX + 20) yesX = modalX + 20;

    DrawRectangle(yesX - 15, buttonY - 10, yw + 30, buttonHeight, Fade(Color{255, 100, 100, 255}, 0.8f));
    DrawRectangleLines(yesX - 15, buttonY - 10, yw + 30, buttonHeight, Color{255, 150, 150, 255});
    DrawTextEx(m_font, yes, Vector2{(float)yesX, (float)buttonY + 2}, buttonFontSize, 2.0f, Color{255, 255, 200, 255});

    // NO button (right)
    int noX = modalX + modalWidth / 2 + static_cast<int>(40 * scaleFactor);
    if (noX + nw + 15 > modalX + modalWidth - 20) noX = modalX + modalWidth - nw - 35;

    DrawRectangle(noX - 15, buttonY - 10, nw + 30, buttonHeight, Fade(Color{100, 150, 100, 255}, 0.8f));
    DrawRectangleLines(noX - 15, buttonY - 10, nw + 30, buttonHeight, Color{150, 200, 150, 255});
    DrawTextEx(m_font, no, Vector2{(float)noX, (float)buttonY + 2}, buttonFontSize, 2.0f, Color{200, 255, 200, 255});

    // Instructions - dynamic sizing
    float instFontSize = 20.0f * scaleFactor;
    if (instFontSize < 16.0f) instFontSize = 16.0f;
    if (instFontSize > 28.0f) instFontSize = 28.0f;

    const char *instructions = "Y/ENTER = Yes    N/ESC = No";
    int iw = MeasureTextEx(m_font, instructions, instFontSize, 2.0f).x;
    int instX = modalX + modalWidth / 2 - iw / 2;
    int instY = modalY + modalHeight - static_cast<int>(30 * scaleFactor);
    if (instY > modalY + modalHeight - 20) instY = modalY + modalHeight - 20;

    DrawTextEx(m_font, instructions, Vector2{(float)instX, (float)instY}, instFontSize, 2.0f, Color{180, 190, 210, 255});
}

void Menu::HandleMapSelection()
{
    // Map selection keyboard handling is now done in HandleMapSelectionKeyboardNavigation()
    // This function is kept for potential future non-keyboard input handling
}

void Menu::UpdatePagination()
{
    if (m_availableMaps.empty())
    {
        m_totalPages = 0;
        m_currentPage = 0;
        return;
    }

    m_totalPages = (static_cast<int>(m_availableMaps.size()) + m_mapsPerPage - 1) / m_mapsPerPage;
    if (m_currentPage >= m_totalPages)
        m_currentPage = std::max(0, m_totalPages - 1);
}

void Menu::NextPage()
{
    if (m_currentPage < m_totalPages - 1)
    {
        m_currentPage++;
        TraceLog(LOG_INFO, "Menu::NextPage() - Advanced to page %d/%d", m_currentPage + 1, m_totalPages);
    }
    else
    {
        m_currentPage = 0; // Wrap around to first page
        TraceLog(LOG_INFO, "Menu::NextPage() - Wrapped to page %d/%d", m_currentPage + 1, m_totalPages);
    }
}

void Menu::PreviousPage()
{
    if (m_currentPage > 0)
    {
        m_currentPage--;
        TraceLog(LOG_INFO, "Menu::PreviousPage() - Moved back to page %d/%d", m_currentPage + 1, m_totalPages);
    }
    else
    {
        m_currentPage = m_totalPages - 1; // Wrap around to last page
        TraceLog(LOG_INFO, "Menu::PreviousPage() - Wrapped to page %d/%d", m_currentPage + 1, m_totalPages);
    }
}

int Menu::GetStartMapIndex() const
{
    return m_currentPage * m_mapsPerPage;
}

int Menu::GetEndMapIndex() const
{
    return std::min(GetStartMapIndex() + m_mapsPerPage, static_cast<int>(m_availableMaps.size()));
}


void Menu::InitializeMaps()
{
    m_availableMaps.clear();
    m_selectedMap = 0;
    m_currentPage = 0;

    // First, scan for all available JSON maps automatically
    ScanForJsonMaps();

    // Then, scan for models in the resources directory and create model-based maps
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    auto models = m_mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (!models.empty())
    {
        TraceLog(LOG_INFO, "Menu::InitializeMaps() - Found %d models in resources directory", models.size());

        // Create map entries for each model
        for (const auto& model : models)
        {
            std::string mapName = "model_" + model.name;
            std::string displayName = model.name + " (Model)";
            std::string description = "Model-based map using " + model.name;

            // Assign color based on model type
            Color modelColor = Color{200, 150, 255, 255}; // Default purple for models
            if (model.name.find("tavern") != std::string::npos)
                modelColor = Color{255, 200, 150, 255}; // Orange for tavern
            else if (model.name.find("stairs") != std::string::npos)
                modelColor = Color{150, 255, 150, 255}; // Green for stairs

            m_availableMaps.push_back({
                mapName,
                displayName,
                description,
                model.path,
                modelColor,
                true
            });

            TraceLog(LOG_INFO, "Menu::InitializeMaps() - Added model-based map: %s", displayName.c_str());
        }
    }

    // If no JSON maps or models found, add a fallback built-in map
    if (m_availableMaps.empty())
    {
        TraceLog(LOG_WARNING, "Menu::InitializeMaps() - No JSON maps or models found, adding fallback built-in map");
        m_availableMaps.push_back({
            "parkour_test",
            "Built-in Parkour",
            "Default parkour level with basic platforming",
            NULL,
            YELLOW,
            true
        });
    }
    else
    {
        TraceLog(LOG_INFO, "Menu::InitializeMaps() - Total maps available: %d (JSON: %d, Models: %d)",
                 m_availableMaps.size(), m_jsonMapsCount, models.size());
    }

    // Initialize pagination
    UpdatePagination();
    TraceLog(LOG_INFO, "Menu::InitializeMaps() - Pagination initialized: %d pages for %d maps",
             m_totalPages, m_availableMaps.size());
}


void Menu::RenderMapSelection() const
{
    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleFactor = screenWidth / 1920.0f;

    // Modern dark theme background with subtle gradient
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{23, 23, 23, 255});

    // Subtle gradient overlay
    for (int i = 0; i < screenHeight; ++i)
    {
        float t = (float)i / screenHeight;
        Color gradientColor = Color{
            (unsigned char)(23 + 10 * t),
            (unsigned char)(23 + 10 * t),
            (unsigned char)(23 + 15 * t),
            255
        };
        DrawLine(0, i, screenWidth, i, gradientColor);
    }

    // Title - dynamic sizing
    float titleFontSize = 56.0f * scaleFactor;
    if (titleFontSize < 40.0f) titleFontSize = 40.0f;
    if (titleFontSize > 80.0f) titleFontSize = 80.0f;

    const char* title = "SELECT MAP";
    int tw = MeasureTextEx(m_font, title, titleFontSize, 2.0f).x;
    int titleY = static_cast<int>(50.0f * scaleFactor);
    if (titleY < 40) titleY = 40;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, title, Vector2{(float)(screenWidth / 2 - tw / 2 + i), (float)titleY + i}, titleFontSize, 2.0f, Fade(Color{100, 150, 255, 255}, 0.3f / i));
    }
    DrawTextEx(m_font, title, Vector2{(float)(screenWidth / 2 - tw / 2), (float)titleY}, titleFontSize, 2.0f, Color{220, 220, 220, 255});

    // Debug info - show current selection and available maps count
    TraceLog(LOG_DEBUG, "Menu::RenderMapSelection() - Available maps: %d, Selected map: %d", m_availableMaps.size(), m_selectedMap);

    if (m_availableMaps.empty())
    {
        const char* noMaps = "No chapters available";
        int nw = MeasureTextEx(m_font, noMaps, 32, 2.0f).x;
        DrawTextEx(m_font, noMaps, Vector2{(float)(screenWidth / 2 - nw / 2), (float)(screenHeight / 2)}, 32, 2.0f, Color{255, 100, 100, 255});
        return;
    }

    // Card-based layout - horizontal arrangement
    const int maxCards = 5; // Allow up to 5 cards
    const int numCards = std::min(maxCards, static_cast<int>(m_availableMaps.size())); // Show available cards
    int cardWidth = static_cast<int>(400 * scaleFactor);
    int cardHeight = static_cast<int>(280 * scaleFactor);
    int spacing = static_cast<int>(60 * scaleFactor);
    int startY = static_cast<int>(140 * scaleFactor);

    // Ensure minimum usable sizes
    if (cardWidth < 300) cardWidth = 300;
    if (cardHeight < 200) cardHeight = 200;
    if (spacing < 40) spacing = 40;
    if (startY < 120) startY = 120;

    int totalWidth = numCards * cardWidth + (numCards - 1) * spacing;
    int startX = (screenWidth - totalWidth) / 2;

    // Get current page map range
    int startMapIndex = GetStartMapIndex();
    int endMapIndex = GetEndMapIndex();

    // Render cards for current page only
    for (int i = startMapIndex; i < endMapIndex; ++i)
    {
        // Calculate position within current page (0-based)
        int pageIndex = i - startMapIndex;
        int x = startX + pageIndex * (cardWidth + spacing);
        int y = startY;

        const MapInfo& map = m_availableMaps[i];
        bool isSelected = (i == m_selectedMap);

        // Card hover/selection animation
        float cardScale = 1.0f;
        if (isSelected)
        {
            cardScale = 1.05f;
        }

        int scaledWidth = static_cast<int>(cardWidth * cardScale);
        int scaledHeight = static_cast<int>(cardHeight * cardScale);
        int cardX = x + (cardWidth - scaledWidth) / 2;
        int cardY = y + (cardHeight - scaledHeight) / 2;

        // Card shadow/glow effect
        if (isSelected)
        {
            for (int g = 8; g >= 2; g -= 2)
            {
                float alpha = 0.4f / (g/2);
                DrawRectangle(cardX - g, cardY - g, scaledWidth + g*2, scaledHeight + g*2,
                             Fade(Color{100, 150, 255, 255}, alpha));
            }
        }

        // Main card background with gradient
        Color cardColor = isSelected ? Color{60, 80, 120, 255} : Color{45, 50, 65, 255};
        for (int cy = 0; cy < scaledHeight; ++cy)
        {
            float t = (float)cy / scaledHeight;
            float intensity = 1.0f - t * 0.2f;
            Color c = {
                (unsigned char)(cardColor.r * intensity),
                (unsigned char)(cardColor.g * intensity),
                (unsigned char)(cardColor.b * intensity),
                cardColor.a
            };
            DrawLine(cardX, cardY + cy, cardX + scaledWidth, cardY + cy, c);
        }

        // Card border
        DrawRectangleLines(cardX, cardY, scaledWidth, scaledHeight, isSelected ? Color{150, 200, 255, 255} : Color{80, 90, 110, 255});

        // Card preview area (larger, more prominent)
        int previewHeight = static_cast<int>(160 * scaleFactor);
        if (previewHeight < 120) previewHeight = 120;
        DrawRectangle(cardX + 20, cardY + 20, scaledWidth - 40, previewHeight, Fade(BLACK, 0.7f));

        // Map name overlay
        std::string mapNameText = map.displayName;
        float chapterFontSize = 18.0f * scaleFactor;
        if (chapterFontSize < 14.0f) chapterFontSize = 14.0f;
        if (chapterFontSize > 24.0f) chapterFontSize = 24.0f;

        int mapNameW = MeasureTextEx(m_font, mapNameText.c_str(), chapterFontSize, 2.0f).x;
        int mapNameX = cardX + 30;
        int mapNameY = cardY + 30;

        DrawTextEx(m_font, mapNameText.c_str(), Vector2{(float)mapNameX + 1, (float)mapNameY + 1}, chapterFontSize, 2.0f, Fade(BLACK, 0.8f));
        DrawTextEx(m_font, mapNameText.c_str(), Vector2{(float)mapNameX, (float)mapNameY}, chapterFontSize, 2.0f, Color{255, 255, 255, 255});

        // Map name (large, prominent)
        float nameFontSize = 28.0f * scaleFactor;
        if (nameFontSize < 20.0f) nameFontSize = 20.0f;
        if (nameFontSize > 36.0f) nameFontSize = 36.0f;

        int nameW = MeasureTextEx(m_font, map.displayName.c_str(), nameFontSize, 2.0f).x;
        int nameX = cardX + scaledWidth / 2 - nameW / 2;
        int nameY = cardY + scaledHeight - static_cast<int>(70 * scaleFactor);
        if (nameY > cardY + scaledHeight - 50) nameY = cardY + scaledHeight - 50;

        DrawTextEx(m_font, map.displayName.c_str(), Vector2{(float)nameX + 1, (float)nameY + 1}, nameFontSize, 2.0f, Fade(BLACK, 0.8f));
        DrawTextEx(m_font, map.displayName.c_str(), Vector2{(float)nameX, (float)nameY}, nameFontSize, 2.0f, Color{255, 255, 255, 255});

        // Map description
        float descFontSize = 16.0f * scaleFactor;
        if (descFontSize < 12.0f) descFontSize = 12.0f;
        if (descFontSize > 22.0f) descFontSize = 22.0f;

        std::string desc = map.description;
        if (desc.length() > static_cast<int>(35 * scaleFactor)) desc = desc.substr(0, static_cast<int>(32 * scaleFactor)) + "...";
        int descW = MeasureTextEx(m_font, desc.c_str(), descFontSize, 2.0f).x;
        int descX = cardX + scaledWidth / 2 - descW / 2;
        int descY = cardY + scaledHeight - static_cast<int>(35 * scaleFactor);
        if (descY > cardY + scaledHeight - 25) descY = cardY + scaledHeight - 25;

        DrawTextEx(m_font, desc.c_str(), Vector2{(float)descX, (float)descY}, descFontSize, 2.0f, Fade(Color{255, 255, 255, 255}, 0.8f));

        // Selection indicator
        if (isSelected)
        {
            DrawRectangleLines(cardX - 2, cardY - 2, scaledWidth + 4, scaledHeight + 4, Color{255, 220, 100, 255});
            DrawRectangleLines(cardX - 4, cardY - 4, scaledWidth + 8, scaledHeight + 8, Fade(Color{255, 220, 100, 255}, 0.5f));
        }
    }

    // Back button (bottom left, like in the image)
    float backFontSize = 24.0f * scaleFactor;
    if (backFontSize < 18.0f) backFontSize = 18.0f;
    if (backFontSize > 32.0f) backFontSize = 32.0f;

    const char* backText = "Back";
    int backW = MeasureTextEx(m_font, backText, backFontSize, 2.0f).x;
    int backX = static_cast<int>(30 * scaleFactor);
    if (backX < 20) backX = 20;
    int backY = screenHeight - static_cast<int>(50 * scaleFactor);
    if (backY > screenHeight - 40) backY = screenHeight - 40;

    // Back button background
    DrawRectangle(backX - 10, backY - 5, backW + 20, static_cast<int>(backFontSize + 10), Fade(Color{80, 60, 60, 255}, 0.8f));
    DrawRectangleLines(backX - 10, backY - 5, backW + 20, static_cast<int>(backFontSize + 10), Color{200, 100, 100, 255});

    DrawTextEx(m_font, backText, Vector2{(float)backX, (float)backY}, backFontSize, 2.0f, Color{255, 150, 150, 255});

    // Pagination indicators - show current page and total pages
    if (m_totalPages > 1)
    {
        float pageFontSize = 20.0f * scaleFactor;
        if (pageFontSize < 16.0f) pageFontSize = 16.0f;
        if (pageFontSize > 28.0f) pageFontSize = 28.0f;

        std::string pageText = TextFormat("Page %d of %d", m_currentPage + 1, m_totalPages);
        int pageW = MeasureTextEx(m_font, pageText.c_str(), pageFontSize, 2.0f).x;
        int pageX = screenWidth / 2 - pageW / 2;
        int pageY = screenHeight - static_cast<int>(80 * scaleFactor);
        if (pageY > screenHeight - 60) pageY = screenHeight - 60;

        // Page indicator background
        DrawRectangle(pageX - 10, pageY - 5, pageW + 20, static_cast<int>(pageFontSize + 10), Fade(Color{0, 0, 0, 255}, 0.6f));
        DrawRectangleLines(pageX - 10, pageY - 5, pageW + 20, static_cast<int>(pageFontSize + 10), Color{100, 120, 140, 255});

        DrawTextEx(m_font, pageText.c_str(), Vector2{(float)pageX, (float)pageY}, pageFontSize, 2.0f, Color{220, 230, 255, 255});
    }

    // Instructions - dynamic sizing
    float instFontSize = 18.0f * scaleFactor;
    if (instFontSize < 14.0f) instFontSize = 14.0f;
    if (instFontSize > 24.0f) instFontSize = 24.0f;

    std::string instructions = "[←→] Navigate Maps   [PgUp/PgDn] Change Page   [Ctrl+←→] Page Nav   [Enter] Select   [Esc] Back";
    int iw = MeasureTextEx(m_font, instructions.c_str(), instFontSize, 2.0f).x;
    int instY = screenHeight - static_cast<int>(25 * scaleFactor);
    if (instY > screenHeight - 20) instY = screenHeight - 20;

    DrawTextEx(m_font, instructions.c_str(), Vector2{(float)(screenWidth / 2 - iw / 2), (float)instY}, instFontSize, 2.0f, Color{150, 160, 180, 255});
}


const MapInfo* Menu::GetSelectedMap() const
{
    if (m_selectedMap >= 0 && m_selectedMap < static_cast<int>(m_availableMaps.size()))
    {
        return &m_availableMaps[m_selectedMap];
    }
    return nullptr;
}

std::string Menu::GetSelectedMapName() const
{
    const MapInfo* selectedMap = GetSelectedMap();
    if (selectedMap)
    {
        // Check if this is a JSON map (indicated by file path starting with "maps/")
        if (selectedMap->name.find("maps/") == 0 || selectedMap->name.find(".json") != std::string::npos)
        {
            // Return the full path for JSON maps
            return PROJECT_ROOT_DIR + selectedMap->name;
        }
        else if (selectedMap->name.find("model_") == 0)
        {
            // This is a model-based map - return the model path
            return selectedMap->previewImage; // previewImage field contains the model path for model-based maps
        }
        else
        {
            // Return the map name for built-in maps
            return selectedMap->name;
        }
    }
    return ""; // Default fallback
}

void Menu::ScanForJsonMaps()
{
    m_jsonMapsCount = 0;

    try
    {
        namespace fs = std::filesystem;
        std::string rootDir = PROJECT_ROOT_DIR;

        // Scan multiple potential map directories
        std::vector<std::string> searchDirectories = {
            rootDir + "/src/Game/Resource",
            rootDir + "/src/Game/Resource/maps",
            rootDir + "/resources/maps",
            rootDir + "/maps"
        };

        TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Scanning for JSON map files...");
        TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Project root directory: %s", rootDir.c_str());

        for (const std::string& dir : searchDirectories)
        {
            TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Checking directory: %s", dir.c_str());

            if (!fs::exists(dir))
            {
                TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Directory does not exist: %s", dir.c_str());
                continue;
            }

            if (!fs::is_directory(dir))
            {
                TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Path is not a directory: %s", dir.c_str());
                continue;
            }

            TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Scanning directory: %s", dir.c_str());

            int filesInDirectory = 0;
            for (const auto& entry : fs::directory_iterator(dir))
            {
                if (!entry.is_regular_file())
                    continue;

                filesInDirectory++;
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Found file: %s", filename.c_str());

                // Look for .json files (excluding system files)
                if (extension == ".json" &&
                    filename != "game.cfg" &&
                    filename != "config.json")
                {
                    std::string mapPath = entry.path().string();
                    // Convert to relative path from project root
                    if (mapPath.find(rootDir) == 0)
                    {
                        mapPath = mapPath.substr(rootDir.length());
                    }

                    // Extract display name from filename (remove .json extension)
                    std::string displayName = filename.substr(0, filename.length() - 5); // Remove .json

                    // Capitalize first letter and improve formatting
                    if (!displayName.empty())
                    {
                        displayName[0] = toupper(displayName[0]);
                    }

                    // Replace underscores with spaces for better readability
                    std::replace(displayName.begin(), displayName.end(), '_', ' ');

                    // Create a more descriptive name
                    std::string fullDisplayName = displayName + " (Map)";

                    // Generate description based on file location or name
                    std::string description = "Custom map";
                    if (mapPath.find("test") != std::string::npos)
                    {
                        description = "Test map for development";
                    }
                    else if (mapPath.find("parkour") != std::string::npos)
                    {
                        description = "Parkour challenge map";
                    }

                    // Assign color based on map type or name
                    Color mapColor = Color{255, 200, 100, 255}; // Default orange
                    if (displayName.find("Test") != std::string::npos)
                    {
                        mapColor = LIME; // Green for test maps
                    }
                    else if (displayName.find("Parkour") != std::string::npos)
                    {
                        mapColor = SKYBLUE; // Blue for parkour maps
                    }

                    m_availableMaps.push_back({
                        mapPath,  // Store the relative path
                        fullDisplayName,
                        description,
                        "/resources/map_previews/custom_map.png",
                        mapColor,
                        true
                    });

                    m_jsonMapsCount++;
                    TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Added map: %s (%s)", fullDisplayName.c_str(), mapPath.c_str());
                }
            }

            TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Directory %s contains %d files", dir.c_str(), filesInDirectory);
        }

        TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Scan completed, found %d maps total", m_availableMaps.size());

        // List all found maps for debugging
        for (size_t i = 0; i < m_availableMaps.size(); ++i)
        {
            const auto& map = m_availableMaps[i];
            TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Map %d: %s -> %s", i, map.displayName.c_str(), map.name.c_str());
        }
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Menu::ScanForJsonMaps() - Error scanning for JSON maps: %s", e.what());
    }
}

void Menu::ToggleConsole()
{
    m_consoleOpen = !m_consoleOpen;
    if (m_consoleOpen)
    {
        m_consoleInput.clear();
        m_consoleHistoryIndex = m_consoleHistory.size();
    }
}

void Menu::HandleConsoleInput()
{
    if (!m_consoleOpen) return;

    int key = GetCharPressed();
    while (key > 0)
    {
        if (key >= 32 && key <= 125) // Printable characters
        {
            m_consoleInput += (char)key;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !m_consoleInput.empty())
    {
        m_consoleInput.pop_back();
    }

    if (IsKeyPressed(KEY_ENTER) && !m_consoleInput.empty())
    {
        // Add to history
        m_consoleHistory.push_back(m_consoleInput);
        if (m_consoleHistory.size() > MAX_HISTORY_LINES)
        {
            m_consoleHistory.erase(m_consoleHistory.begin());
        }

        // Execute command
        ExecuteConsoleCommand(m_consoleInput);

        // Clear input
        m_consoleInput.clear();
        m_consoleHistoryIndex = m_consoleHistory.size();
    }

    if (IsKeyPressed(KEY_UP) && !m_consoleHistory.empty())
    {
        if (m_consoleHistoryIndex > 0)
        {
            m_consoleHistoryIndex--;
            m_consoleInput = m_consoleHistory[m_consoleHistoryIndex];
        }
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        if (m_consoleHistoryIndex < m_consoleHistory.size() - 1)
        {
            m_consoleHistoryIndex++;
            m_consoleInput = m_consoleHistory[m_consoleHistoryIndex];
        }
        else if (m_consoleHistoryIndex == m_consoleHistory.size() - 1)
        {
            m_consoleHistoryIndex = m_consoleHistory.size();
            m_consoleInput.clear();
        }
    }
}

void Menu::ExecuteConsoleCommand(const std::string& command)
{
    AddConsoleOutput("] " + command);

    // Parse command and arguments
    std::string cmd = command;
    std::string args;

    size_t spacePos = command.find(' ');
    if (spacePos != std::string::npos)
    {
        cmd = command.substr(0, spacePos);
        args = command.substr(spacePos + 1);
    }

    // Convert to lowercase for case-insensitive comparison
    for (char& c : cmd) c = std::tolower(c);

    // Execute command using switch statement
    switch (cmd[0]) // Use first character for faster lookup
    {
        case 'h':
            if (cmd == "help")
            {
                AddConsoleOutput("Available commands:");
                AddConsoleOutput("  help - Show this help");
                AddConsoleOutput("  clear - Clear console");
                AddConsoleOutput("  quit/exit - Exit game");
                AddConsoleOutput("  map <name> - Load map");
                AddConsoleOutput("  fps - Show current FPS");
                AddConsoleOutput("  res <width>x<height> - Set resolution");
                AddConsoleOutput("  fullscreen - Toggle fullscreen");
                AddConsoleOutput("  vsync <on/off> - Toggle VSync");
                AddConsoleOutput("  savecfg - Save current settings");
                AddConsoleOutput("  noclip - Toggle noclip mode");
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'c':
            if (cmd == "clear")
            {
                m_consoleOutput.clear();
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'q':
        case 'e':
            if (cmd == "quit" || cmd == "exit")
            {
                if (m_engine)
                {
                    m_engine->RequestExit();
                }
                AddConsoleOutput("Exiting game...");
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'f':
            if (cmd == "fps")
            {
                int fps = GetFPS();
                AddConsoleOutput("Current FPS: " + std::to_string(fps));
            }
            else if (cmd == "fullscreen")
            {
                if (IsWindowFullscreen())
                {
                    ToggleFullscreen();
                    AddConsoleOutput("Switched to windowed mode");
                }
                else
                {
                    ToggleFullscreen();
                    AddConsoleOutput("Switched to fullscreen mode");
                }
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'm':
            if (cmd == "map")
            {
                if (!args.empty())
                {
                    AddConsoleOutput("Loading map: " + args);
                    // TODO: Implement map loading
                }
                else
                {
                    AddConsoleOutput("Usage: map <mapname>");
                }
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'r':
            if (cmd == "res")
            {
                int width, height;
                if (sscanf(args.c_str(), "%dx%d", &width, &height) == 2)
                {
                    SetWindowSize(width, height);
                    AddConsoleOutput("Resolution set to " + std::to_string(width) + "x" + std::to_string(height));
                }
                else
                {
                    AddConsoleOutput("Usage: res <width>x<height>");
                }
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'v':
            if (cmd == "vsync")
            {
                if (args == "on" || args == "1")
                {
                    SetWindowState(FLAG_VSYNC_HINT);
                    m_config.SetVSync(true);
                    AddConsoleOutput("VSync enabled");
                }
                else if (args == "off" || args == "0")
                {
                    ClearWindowState(FLAG_VSYNC_HINT);
                    m_config.SetVSync(false);
                    AddConsoleOutput("VSync disabled");
                }
                else
                {
                    AddConsoleOutput("Usage: vsync <on/off>");
                }
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 's':
            if (cmd == "savecfg")
            {
                SaveSettings();
                AddConsoleOutput("Settings saved to game.cfg");
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        case 'n':
            if (cmd == "noclip")
            {
                AddConsoleOutput("Noclip mode toggled (not implemented yet)");
                // TODO: Implement noclip mode
            }
            else
            {
                AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            }
            break;

        default:
            AddConsoleOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
            break;
    }
}

void Menu::AddConsoleOutput(const std::string& text)
{
    m_consoleOutput.push_back(text);
    if (m_consoleOutput.size() > MAX_CONSOLE_LINES)
    {
        m_consoleOutput.erase(m_consoleOutput.begin());
    }
}

void Menu::RenderConsole() const
{
    if (!m_consoleOpen) return;

    const int consoleHeight = GetScreenHeight() / 2;
    const int lineHeight = 20;

    // Semi-transparent overlay to indicate game is paused
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.3f));

    // Background
    DrawRectangle(0, 0, GetScreenWidth(), consoleHeight, Fade(BLACK, 0.8f));
    DrawRectangleLines(0, 0, GetScreenWidth(), consoleHeight, WHITE);

    // Title
    DrawTextEx(m_font, "Console", Vector2{10.0f, 10.0f}, 20, 2.0f, YELLOW);

    // Pause indicator
    DrawTextEx(m_font, "GAME PAUSED", Vector2{(float)(GetScreenWidth() - 150), 10.0f}, 16, 2.0f, RED);

    // Output lines
    int startY = 40;
    size_t startLine = (m_consoleOutput.size() > (size_t)(consoleHeight - 80) / lineHeight) ?
                      m_consoleOutput.size() - (consoleHeight - 80) / lineHeight : 0;

    for (size_t i = startLine; i < m_consoleOutput.size(); ++i)
    {
        int y = startY + (i - startLine) * lineHeight;
        DrawTextEx(m_font, m_consoleOutput[i].c_str(), Vector2{10.0f, (float)y}, 16, 2.0f, WHITE);
    }

    // Input line
    int inputY = consoleHeight - 30;
    DrawTextEx(m_font, "]", Vector2{10.0f, (float)inputY}, 16, 2.0f, GREEN);
    DrawTextEx(m_font, m_consoleInput.c_str(), Vector2{25.0f, (float)inputY}, 16, 2.0f, WHITE);

    // Blinking cursor
    if ((int)(GetTime() * 2) % 2 == 0)
    {
        int cursorX = 25 + MeasureTextEx(m_font, m_consoleInput.c_str(), 16, 2.0f).x;
        DrawTextEx(m_font, "_", Vector2{(float)cursorX, (float)inputY}, 16, 2.0f, WHITE);
    }

    // Instructions
    const char* instructions = "[~] Toggle Console [↑/↓] History [Enter] Execute";
    int iw = MeasureTextEx(m_font, instructions, 14, 2.0f).x;
    DrawTextEx(m_font, instructions, Vector2{(float)(GetScreenWidth() - iw - 10), (float)(GetScreenHeight() - 20)}, 14, 2.0f, GRAY);
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
