#include "Menu.h"
#include "Engine/Engine.h"
#include <cstdio> // sscanf
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <string>
#include <vector>

// Include raylib window functions
#ifdef _WIN32
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#endif

Menu::Menu()
{
    // Load configuration file
    //LoadSettings();

    // Main Menu
    m_mainMenu = {{"Start Game", MenuAction::StartGame},
                  {"Options", MenuAction::OpenOptions},
                  {"Mods", MenuAction::OpenMods},
                  {"Credits", MenuAction::OpenCredits},
                  {"Quit", MenuAction::ExitGame}};

    // Options Menu
    m_optionsMenu = {{"Video", MenuAction::OpenVideoMode},
                     {"Audio", MenuAction::OpenAudio},
                     {"Controls", MenuAction::OpenControls},
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

    // Audio Menu
    m_audioMenu = {{"Master Volume", MenuAction::None},
                   {"Music Volume", MenuAction::None},
                   {"SFX Volume", MenuAction::None},
                   {"Back", MenuAction::BackToMainMenu}};

    // Controls Menu
    m_controlsMenu = {{"Rebind Keys", MenuAction::None},
                      {"Invert Y Axis", MenuAction::None},
                      {"Back", MenuAction::BackToMainMenu}};

    m_currentMenu = &m_mainMenu;
    m_buttonScales.assign(m_currentMenu->size(), 1.0f);
}

float Menu::Lerp(float a, float b, float t) const { return a + (b - a) * t; }

MenuAction Menu::GetAction() const { return m_action; }

void Menu::ResetAction() { m_action = MenuAction::None; }

void Menu::GetEngine(Engine *engine) { m_engine = engine; }

void Menu::Update()
{
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
        break;
    case MenuState::Controls:
        m_currentMenu = &m_controlsMenu;
        break;
    default:
        m_currentMenu = nullptr;
        break;
    }

    if (m_currentMenu && m_selected >= static_cast<int>(m_currentMenu->size()))
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
    case MenuAction::SinglePlayer:
        m_state = MenuState::MapSelection;
        InitializeMaps();
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
                   m_state == MenuState::Controls)
                      ? MenuState::Options
                      : MenuState::Main;
        ResetAction();
        break;
    case MenuAction::ExitGame:
        if (m_state == MenuState::ConfirmExit && m_engine)
            m_engine->RequestExit();
        else
            m_state = MenuState::ConfirmExit;
        ResetAction();
        break;
    case MenuAction::StartGameWithMap:
    case MenuAction::SelectMap1:
    case MenuAction::SelectMap2:
    case MenuAction::SelectMap3:
        // These actions are handled by the Game system, don't reset them here
        // The Game system will handle the transition and reset the action
        break;
    default:
        ResetAction();
        break;
    }
}

void Menu::HandleConfirmExit()
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

void Menu::HandleMainMenuMouseSelection(Vector2 mousePos, bool clicked)
{
    if (!m_currentMenu)
        return;

    constexpr int kBtnW = 200, kBtnH = 50, kStartY = 300, kSpacing = 70;

    for (size_t i = 0; i < m_currentMenu->size(); ++i)
    {
        int x = GetScreenWidth() / 2 - kBtnW / 2;
        int y = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle rect = {(float)x, (float)y, (float)kBtnW, (float)kBtnH};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                m_action = (*m_currentMenu)[m_selected].action;
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

    // Calculate layout
    const int mapsPerRow = 3;
    const int mapWidth = 280;
    const int mapHeight = 200;
    const int spacing = 40;
    const int startY = 120;

    int totalWidth = mapsPerRow * mapWidth + (mapsPerRow - 1) * spacing;
    int startX = (GetScreenWidth() - totalWidth) / 2;

    // Check each map for mouse interaction
    for (size_t i = 0; i < m_availableMaps.size(); ++i)
    {
        int row = static_cast<int>(i) / mapsPerRow;
        int col = static_cast<int>(i) % mapsPerRow;

        int x = startX + col * (mapWidth + spacing);
        int y = startY + row * (mapHeight + spacing);
        Rectangle rect = {(float)x, (float)y, (float)mapWidth, (float)mapHeight};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selectedMap = static_cast<int>(i);
            if (clicked)
            {
                // Start the game with selected map
                const MapInfo* selectedMap = GetSelectedMap();
                if (selectedMap)
                {
                    std::cout << "Starting game with map: " << selectedMap->name << std::endl;
                    // Set action to start game with selected map
                    switch (m_selectedMap)
                    {
                        case 0:
                            m_action = MenuAction::SelectMap1;
                            break;
                        case 1:
                            m_action = MenuAction::SelectMap2;
                            break;
                        case 2:
                            m_action = MenuAction::SelectMap3;
                            break;
                        default:
                            m_action = MenuAction::StartGameWithMap;
                            break;
                    }
                }
            }
            break;
        }
    }
}

void Menu::HandleConfirmExitMouseSelection(Vector2 mousePos, bool clicked)
{
    if (!clicked)
        return;

    // Modal dialog container
    int modalWidth = 500;
    int modalHeight = 300;
    int modalX = GetScreenWidth() / 2 - modalWidth / 2;
    int modalY = GetScreenHeight() / 2 - modalHeight / 2;

    // YES button (left)
    int yesX = modalX + modalWidth / 2 - 100;
    int buttonY = modalY + modalHeight - 80;
    Rectangle yesRect = {(float)(yesX - 15), (float)(buttonY - 10), 80.0f, 40.0f};

    // NO button (right)
    int noX = modalX + modalWidth / 2 + 40;
    Rectangle noRect = {(float)(noX - 15), (float)(buttonY - 10), 80.0f, 40.0f};

    if (CheckCollisionPointRec(mousePos, yesRect))
    {
        m_action = MenuAction::ExitGame;
        if (m_engine)
            m_engine->RequestExit();
    }
    else if (CheckCollisionPointRec(mousePos, noRect))
    {
        m_state = MenuState::Main;
    }
}

void Menu::HandleMainMenuKeyboardNavigation()
{
    if (!m_currentMenu)
        return;

    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_currentMenu->size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_currentMenu->size() - 1) % m_currentMenu->size();

    if (IsKeyPressed(KEY_ENTER))
        m_action = (*m_currentMenu)[m_selected].action;

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

    const int mapsPerRow = 3;
    int currentRow = m_selectedMap / mapsPerRow;
    int currentCol = m_selectedMap % mapsPerRow;
    int totalRows = (static_cast<int>(m_availableMaps.size()) + mapsPerRow - 1) / mapsPerRow;

    if (IsKeyPressed(KEY_DOWN) && currentRow < totalRows - 1)
    {
        int newRow = currentRow + 1;
        int newIndex = newRow * mapsPerRow + currentCol;
        if (newIndex < static_cast<int>(m_availableMaps.size()))
            m_selectedMap = newIndex;
    }
    if (IsKeyPressed(KEY_UP) && currentRow > 0)
    {
        int newRow = currentRow - 1;
        int newIndex = newRow * mapsPerRow + currentCol;
        if (newIndex < static_cast<int>(m_availableMaps.size()))
            m_selectedMap = newIndex;
    }
    if (IsKeyPressed(KEY_LEFT) && currentCol > 0)
    {
        m_selectedMap--;
    }
    if (IsKeyPressed(KEY_RIGHT) && currentCol < mapsPerRow - 1)
    {
        int newIndex = currentRow * mapsPerRow + currentCol + 1;
        if (newIndex < static_cast<int>(m_availableMaps.size()))
            m_selectedMap = newIndex;
    }

    if (IsKeyPressed(KEY_ENTER) && !m_availableMaps.empty())
    {
        // Start the game with selected map
        const MapInfo* selectedMap = GetSelectedMap();
        if (selectedMap)
        {
            std::cout << "Starting game with map: " << selectedMap->name << std::endl;
            // Set action to start game with selected map
            switch (m_selectedMap)
            {
                case 0:
                    m_action = MenuAction::SelectMap1;
                    break;
                case 1:
                    m_action = MenuAction::SelectMap2;
                    break;
                case 2:
                    m_action = MenuAction::SelectMap3;
                    break;
                default:
                    m_action = MenuAction::StartGameWithMap;
                    break;
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::GameMode;
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

void Menu::Render() const {
    // Modern gradient background with multiple layers
    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = (float)i / GetScreenHeight();

        // Main gradient background
        Color gradientColor = Color{
            (unsigned char)(10 + 15 * t),    // R: Dark blue to lighter
            (unsigned char)(20 + 40 * t),    // G: Dark purple to lighter
            (unsigned char)(50 + 80 * t),    // B: Dark to lighter blue
            255
        };
        DrawLine(0, i, GetScreenWidth(), i, gradientColor);
    }

    // Add subtle animated overlay pattern
    static float time = 0.0f;
    time += GetFrameTime();

    for (int i = 0; i < 20; i++)
    {
        float y = (GetScreenHeight() / 20) * i + sinf(time + i * 0.5f) * 10;
        DrawLine(0, (int)y, GetScreenWidth(), (int)y,
                 Fade(Color{100, 150, 255, 255}, 0.1f + sinf(time * 2 + i) * 0.05f));
    }

    // Dark overlay for better text readability
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.3f));

    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        RenderMenu();
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

void Menu::RenderMenu() const
{
    if (!m_currentMenu)
        return;

    Vector2 mousePos = GetMousePosition();
    constexpr int kBtnW = 280, kBtnH = 65, kStartY = 320, kSpacing = 85;

    // Modern title with glow effect
    const char *title = (m_state == MenuState::Main) ? "CHAINED DECOS" : (*m_currentMenu)[m_selected].label;

    // Draw title glow/shadow
    int tw = MeasureText(title, 60);
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Multiple glow layers for depth
    for (int i = 3; i >= 1; i--)
    {
        DrawText(title, titleX + i, titleY + i, 60, Fade(Color{0, 100, 255, 255}, 0.3f / i));
    }

    // Main title with gradient effect
    DrawText(title, titleX, titleY, 60, Color{255, 255, 255, 255});

    // Subtitle for main menu
    if (m_state == MenuState::Main)
    {
        const char *subtitle = "Modern 3D Platformer";
        int stw = MeasureText(subtitle, 24);
        DrawText(subtitle, GetScreenWidth() / 2 - stw / 2, titleY + 50, 24, Fade(Color{150, 200, 255, 255}, 0.8f));

        // Version info in bottom corner
        const char *version = "v1.0.0";
        int vw = MeasureText(version, 16);
        DrawText(version, GetScreenWidth() - vw - 20, GetScreenHeight() - 25, 16, Fade(Color{120, 140, 160, 255}, 0.7f));
    }

    if (m_buttonScales.size() != m_currentMenu->size())
        m_buttonScales.assign(m_currentMenu->size(), 1.0f);

    for (size_t i = 0; i < m_currentMenu->size(); ++i)
    {
        const auto &item = (*m_currentMenu)[i];
        int baseX = GetScreenWidth() / 2 - kBtnW / 2;
        int baseY = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle rect = {(float)baseX, (float)baseY, (float)kBtnW, (float)kBtnH};

        bool hovered = CheckCollisionPointRec(mousePos, rect);
        bool selected = (static_cast<int>(i) == m_selected);
        float targetScale = (hovered || selected) ? 1.15f : 1.0f; // Increased scale for more dramatic effect
        m_buttonScales[i] = Lerp(m_buttonScales[i], targetScale, 0.2f); // Faster animation

        int w = static_cast<int>(kBtnW * m_buttonScales[i]);
        int h = static_cast<int>(kBtnH * m_buttonScales[i]);
        int x = GetScreenWidth() / 2 - w / 2;
        int y = baseY - (h - kBtnH) / 2;
        Rectangle btnRect = {(float)x, (float)y, (float)w, (float)h};

        // Modern button design with multiple layers
        Color baseColor, accentColor, glowColor;

        if (selected)
        {
            baseColor = {255, 200, 100, 255};    // Bright gold
            accentColor = {255, 150, 50, 255};   // Orange accent
            glowColor = {255, 255, 150, 200};    // Yellow glow
        }
        else if (hovered)
        {
            baseColor = {200, 220, 255, 255};    // Light blue
            accentColor = {150, 200, 255, 255};  // Blue accent
            glowColor = {150, 200, 255, 150};    // Blue glow
        }
        else
        {
            baseColor = {180, 190, 210, 255};    // Neutral gray-blue
            accentColor = {140, 150, 170, 255};  // Darker accent
            glowColor = {0, 0, 0, 0};           // No glow
        }

        // Draw glow effect for selected/hovered buttons
        if (selected || hovered)
        {
            for (int g = 8; g >= 2; g -= 2)
            {
                float alpha = (selected) ? 0.4f / (g/2) : 0.2f / (g/2);
                DrawRectangle(x - g, y - g, w + g*2, h + g*2,
                             Fade(glowColor, alpha));
            }
        }

        // Main button background with gradient
        for (int j = 0; j < h; ++j)
        {
            float t = (float)j / h;
            float intensity = 1.0f - t * 0.3f; // Darker at bottom
            Color c = {
                (unsigned char)(baseColor.r * intensity),
                (unsigned char)(baseColor.g * intensity),
                (unsigned char)(baseColor.b * intensity),
                baseColor.a
            };
            DrawLine(x, y + j, x + w, y + j, c);
        }

        // Highlight/shine effect
        DrawRectangle(x + 2, y + 2, w - 4, h/3, Fade(WHITE, 0.3f));

        // Modern border with rounded corners effect
        if (selected)
        {
            DrawRectangleLinesEx(btnRect, 3, accentColor);
            // Double border for selected
            DrawRectangleLinesEx(Rectangle{btnRect.x - 2, btnRect.y - 2, btnRect.width + 4, btnRect.height + 4}, 1, Fade(accentColor, 0.5f));
        }
        else if (hovered)
        {
            DrawRectangleLinesEx(btnRect, 2, accentColor);
        }
        else
        {
            DrawRectangleLinesEx(btnRect, 1, Color{120, 130, 150, 255});
        }

        // Modern text with better font and effects
        int textSize = selected ? 32 : (hovered ? 30 : 28);
        Color textColor = selected ? Color{50, 50, 80, 255} : (hovered ? Color{30, 30, 50, 255} : Color{40, 40, 60, 255});

        int textW = MeasureText(item.label, textSize);
        int textX = x + w / 2 - textW / 2;
        int textY = y + h / 2 - textSize / 2;

        // Text shadow for depth
        DrawText(item.label, textX + 2, textY + 2, textSize, Fade(BLACK, 0.5f));

        // Main text with modern color
        DrawText(item.label, textX, textY, textSize, textColor);
    }

    // Modern footer with better styling
    const char *footer = "ENTER Select    ESC Back    ↑↓ Navigate    MOUSE Click";
    int fw = MeasureText(footer, 20);
    int footerX = GetScreenWidth() / 2 - fw / 2;
    int footerY = GetScreenHeight() - 35;

    // Footer background
    DrawRectangle(footerX - 10, footerY - 5, fw + 20, 30, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 10, footerY - 5, fw + 20, 30, Fade(Color{100, 120, 140, 255}, 0.5f));

    // Modern footer text with color coding
    DrawText("ENTER", footerX, footerY, 20, Color{150, 255, 150, 255});
    DrawText(" Select    ", footerX + 70, footerY, 20, Color{200, 200, 200, 255});
    DrawText("ESC", footerX + 150, footerY, 20, Color{255, 150, 150, 255});
    DrawText(" Back    ", footerX + 185, footerY, 20, Color{200, 200, 200, 255});
    DrawText("↑↓", footerX + 245, footerY, 20, Color{150, 150, 255, 255});
    DrawText(" Navigate    ", footerX + 275, footerY, 20, Color{200, 200, 200, 255});
    DrawText("MOUSE", footerX + 380, footerY, 20, Color{255, 200, 100, 255});
    DrawText(" Click", footerX + 460, footerY, 20, Color{200, 200, 200, 255});
}

void Menu::RenderSettingsMenu() const {
    int startY = 150, spacing = 80, fontSize = 30;

    // Modern settings title with glow
    const char* settingsTitle = "SETTINGS";
    int titleW = MeasureText(settingsTitle, 45);
    int titleX = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawText(settingsTitle, titleX + i, 45 + i, 45, Fade(Color{255, 150, 50, 255}, 0.5f / i));
    }
    DrawText(settingsTitle, titleX, 45, 45, Color{255, 200, 100, 255});

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
        int labelW = MeasureText(label, fontSize);
        DrawText(label, 80, y + 5, fontSize, labelColor);

        if (!opt.values.empty())
        {
            // Get current system value for display
            std::string currentValue = GetCurrentSettingValue(opt.label);

            // Show current value (smaller font, different position)
            if (!currentValue.empty())
            {
                int currentWidth = MeasureText(currentValue.c_str(), fontSize - 8);
                DrawText(currentValue.c_str(), 80 + 320, y + 8, fontSize - 8,
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
                int textWidth = MeasureText(displayValue.c_str(), fontSize);
                int xPos = GetScreenWidth() - textWidth - 100;

                if (isSelected)
                {
                    // Background for selected value
                    DrawRectangle(xPos - 10, y - 2, textWidth + 20, fontSize + 8, Fade(Color{255, 200, 100, 255}, 0.3f));
                    DrawRectangleLines(xPos - 10, y - 2, textWidth + 20, fontSize + 8, Color{255, 180, 80, 255});
                }

                DrawText(displayValue.c_str(), xPos, y + 5, fontSize,
                          isSelected ? Color{255, 255, 180, 255} : Color{220, 230, 200, 255});
            }
        }
    }

    // Modern settings footer
    std::string footer = "ENTER Apply/Select    ←→ Change    ↑↓ Navigate    ESC Back";
    int fw = MeasureText(footer.c_str(), 18);
    int footerX = GetScreenWidth() / 2 - fw / 2;
    int footerY = GetScreenHeight() - 30;

    // Footer background
    DrawRectangle(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    // Color-coded footer text
    DrawText("ENTER", footerX, footerY, 18, Color{150, 255, 150, 255});
    DrawText(" Apply/Select    ", footerX + 65, footerY, 18, Color{200, 200, 200, 255});
    DrawText("←→", footerX + 190, footerY, 18, Color{150, 150, 255, 255});
    DrawText(" Change    ", footerX + 210, footerY, 18, Color{200, 200, 200, 255});
    DrawText("↑↓", footerX + 290, footerY, 18, Color{150, 150, 255, 255});
    DrawText(" Navigate    ", footerX + 310, footerY, 18, Color{200, 200, 200, 255});
    DrawText("ESC", footerX + 410, footerY, 18, Color{255, 150, 150, 255});
    DrawText(" Back", footerX + 440, footerY, 18, Color{200, 200, 200, 255});
}

void Menu::RenderCredits()
{
    // Modern credits title with glow
    const char *title = "CREDITS";
    int tw = MeasureText(title, 50);
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawText(title, titleX + i, titleY + i, 50, Fade(Color{255, 150, 100, 255}, 0.5f / i));
    }
    DrawText(title, titleX, titleY, 50, Color{255, 200, 150, 255});

    // Modern credits content with better layout
    int y = 180, fs = 28;

    // Developer credit with modern styling
    DrawText("DEVELOPER", 80, y, 24, Color{200, 220, 255, 255});
    DrawText("I#Oleg", 80, y + 30, fs, Color{255, 255, 200, 255});

    y += 100;
    DrawText("ENGINE", 80, y, 24, Color{200, 220, 255, 255});
    DrawText("raylib + rlImGui", 80, y + 30, fs, Color{255, 255, 200, 255});

    y += 100;
    DrawText("UI DESIGN", 80, y, 24, Color{200, 220, 255, 255});
    DrawText("Modern Interface", 80, y + 30, fs, Color{255, 255, 200, 255});

    // Modern footer
    const char *footer = "ESC Back";
    int fw2 = MeasureText(footer, 20);
    int footerX = GetScreenWidth() / 2 - fw2 / 2;
    int footerY = GetScreenHeight() - 30;

    DrawRectangle(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    DrawText("ESC", footerX, footerY, 20, Color{255, 150, 150, 255});
    DrawText(" Back", footerX + 35, footerY, 20, Color{200, 200, 200, 255});
}

void Menu::RenderMods()
{
    // Modern mods title with glow
    const char *title = "MODS";
    int tw = MeasureText(title, 50);
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawText(title, titleX + i, titleY + i, 50, Fade(Color{200, 100, 255, 255}, 0.5f / i));
    }
    DrawText(title, titleX, titleY, 50, Color{220, 150, 255, 255});

    // Modern content layout
    int y = 180, fs = 26;

    // No mods message with modern styling
    const char* noModsMsg = "NO MODS DETECTED";
    int noModsW = MeasureText(noModsMsg, 28);
    int noModsX = GetScreenWidth() / 2 - noModsW / 2;
    DrawText(noModsMsg, noModsX, y, 28, Color{255, 200, 150, 255});

    y += 80;
    const char* instructionMsg = "Place your mods in the 'resources/mods' folder";
    int instructionW = MeasureText(instructionMsg, fs);
    int instructionX = GetScreenWidth() / 2 - instructionW / 2;
    DrawText(instructionMsg, instructionX, y, fs, Color{180, 200, 220, 255});

    // Modern footer
    const char *footer = "ESC Back";
    int fw2 = MeasureText(footer, 20);
    int footerX = GetScreenWidth() / 2 - fw2 / 2;
    int footerY = GetScreenHeight() - 30;

    DrawRectangle(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{120, 140, 160, 255}, 0.5f));

    DrawText("ESC", footerX, footerY, 20, Color{255, 150, 150, 255});
    DrawText(" Back", footerX + 35, footerY, 20, Color{200, 200, 200, 255});
}

void Menu::RenderConfirmExit()
{
    // Modern modal background with blur effect
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Color{0, 0, 0, 180}, 0.7f));

    // Modal dialog container
    int modalWidth = 500;
    int modalHeight = 300;
    int modalX = GetScreenWidth() / 2 - modalWidth / 2;
    int modalY = GetScreenHeight() / 2 - modalHeight / 2;

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

    // Modern title
    const char *msg = "EXIT GAME?";
    int tw = MeasureText(msg, 40);
    int titleX = modalX + modalWidth / 2 - tw / 2;
    int titleY = modalY + 40;

    // Title glow
    for (int i = 2; i >= 1; i--)
    {
        DrawText(msg, titleX + i, titleY + i, 40, Fade(Color{255, 100, 100, 255}, 0.6f / i));
    }
    DrawText(msg, titleX, titleY, 40, Color{255, 150, 150, 255});

    // Modern buttons with better styling
    const char *yes = "YES";
    const char *no = "NO";
    int yw = MeasureText(yes, 28);
    int nw = MeasureText(no, 28);

    int buttonY = modalY + modalHeight - 80;

    // YES button (left)
    int yesX = modalX + modalWidth / 2 - yw - 40;
    DrawRectangle(yesX - 15, buttonY - 10, yw + 30, 40, Fade(Color{255, 100, 100, 255}, 0.8f));
    DrawRectangleLines(yesX - 15, buttonY - 10, yw + 30, 40, Color{255, 150, 150, 255});
    DrawText(yes, yesX, buttonY + 2, 28, Color{255, 255, 200, 255});

    // NO button (right)
    int noX = modalX + modalWidth / 2 + 40;
    DrawRectangle(noX - 15, buttonY - 10, nw + 30, 40, Fade(Color{100, 150, 100, 255}, 0.8f));
    DrawRectangleLines(noX - 15, buttonY - 10, nw + 30, 40, Color{150, 200, 150, 255});
    DrawText(no, noX, buttonY + 2, 28, Color{200, 255, 200, 255});

    // Instructions
    const char *instructions = "Y/ENTER = Yes    N/ESC = No";
    int iw = MeasureText(instructions, 20);
    int instX = modalX + modalWidth / 2 - iw / 2;
    int instY = modalY + modalHeight - 30;

    DrawText(instructions, instX, instY, 20, Color{180, 190, 210, 255});
}

void Menu::HandleMapSelection()
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        m_state = MenuState::GameMode;
        return;
    }

    if (IsKeyPressed(KEY_LEFT))
    {
        m_selectedMap = (m_selectedMap - 1 + static_cast<int>(m_availableMaps.size())) % m_availableMaps.size();
    }
    if (IsKeyPressed(KEY_RIGHT))
    {
        m_selectedMap = (m_selectedMap + 1) % m_availableMaps.size();
    }

    if (IsKeyPressed(KEY_ENTER) && !m_availableMaps.empty())
    {
        // Start the game with selected map
        const MapInfo* selectedMap = GetSelectedMap();
        if (selectedMap)
        {
            std::cout << "Starting game with map: " << selectedMap->name << std::endl;
            // Set action to start game with selected map
            switch (m_selectedMap)
            {
                case 0:
                    m_action = MenuAction::SelectMap1;
                    break;
                case 1:
                    m_action = MenuAction::SelectMap2;
                    break;
                case 2:
                    m_action = MenuAction::SelectMap3;
                    break;
                default:
                    m_action = MenuAction::StartGameWithMap;
                    break;
            }
        }
    }
}

void Menu::InitializeMaps()
{
    m_availableMaps.clear();
    m_selectedMap = 0;

    // Parkour Test Map 1 - Basic Shapes
    m_availableMaps.push_back({
        "parkour_shapes_basic",
        "Basic Shapes Parkour",
        "Learn parkour fundamentals with cubes, spheres, and platforms",
        "/resources/map_previews/parkour_shapes_basic.png",
        SKYBLUE,
        true
    });

    // Parkour Test Map 2 - Geometric Challenge
    m_availableMaps.push_back({
        "parkour_geometric",
        "Geometric Challenge",
        "Advanced parkour with complex geometric arrangements",
        "/resources/map_previews/parkour_geometric.png",
        LIME,
        true
    });

    // Parkour Test Map 3 - Precision Platforming
    m_availableMaps.push_back({
        "parkour_precision",
        "Precision Platforming",
        "Test your precision with small platforms and tight jumps",
        "/resources/map_previews/parkour_precision.png",
        YELLOW,
        true
    });

    // Parkour Test Map 4 - Vertical Challenge
    m_availableMaps.push_back({
        "parkour_vertical",
        "Vertical Ascension",
        "Climb to new heights with vertical raylib shape challenges",
        "/resources/map_previews/parkour_vertical.png",
        PURPLE,
        true
    });

    // Parkour Test Map 5 - Speed Run
    m_availableMaps.push_back({
        "parkour_speedrun",
        "Speed Runner's Gauntlet",
        "Fast-paced parkour course with moving platforms",
        "/resources/map_previews/parkour_speedrun.png",
        ORANGE,
        true
    });

    // Training Ground - Beginner Map
    m_availableMaps.push_back({
        "training_shapes",
        "Shape Training Ground",
        "Learn basic parkour mechanics with simple raylib shapes",
        "/resources/map_previews/training_shapes.png",
        GREEN,
        true
    });
}

void Menu::RenderMapSelection() const
{
    // Background
    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = (float)i / GetScreenHeight();
        DrawLine(0, i, GetScreenWidth(), i,
                 Color{(unsigned char)(15 + 25 * t), (unsigned char)(15 + 30 * t),
                       (unsigned char)(40 + 90 * t), 255});
    }
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));

    // Title
    const char* title = "Select Map";
    int tw = MeasureText(title, 56);
    DrawText(title, GetScreenWidth() / 2 - tw / 2 + 4, 54, 56, Fade(BLACK, 0.75f));
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 50, 56, RAYWHITE);

    if (m_availableMaps.empty())
    {
        const char* noMaps = "No maps available";
        int nw = MeasureText(noMaps, 32);
        DrawText(noMaps, GetScreenWidth() / 2 - nw / 2, GetScreenHeight() / 2, 32, RED);
        return;
    }

    // Calculate layout
    const int mapsPerRow = 3;
    const int mapWidth = 280;
    const int mapHeight = 200;
    const int spacing = 40;
    const int startY = 120;

    int totalWidth = mapsPerRow * mapWidth + (mapsPerRow - 1) * spacing;
    int startX = (GetScreenWidth() - totalWidth) / 2;

    // Render maps in grid layout
    for (size_t i = 0; i < m_availableMaps.size(); ++i)
    {
        int row = static_cast<int>(i) / mapsPerRow;
        int col = static_cast<int>(i) % mapsPerRow;

        int x = startX + col * (mapWidth + spacing);
        int y = startY + row * (mapHeight + spacing);

        const MapInfo& map = m_availableMaps[i];
        bool isSelected = (static_cast<int>(i) == m_selectedMap);

        // Map background
        Color bgColor = isSelected ? map.themeColor : Fade(map.themeColor, 0.3f);
        DrawRectangle(x, y, mapWidth, mapHeight, bgColor);
        DrawRectangleLines(x, y, mapWidth, mapHeight, isSelected ? WHITE : Fade(WHITE, 0.5f));

        // Map preview (placeholder for now)
        DrawRectangle(x + 10, y + 10, mapWidth - 20, mapHeight - 60, Fade(BLACK, 0.5f));

        // Map name
        int nameW = MeasureText(map.displayName.c_str(), 24);
        int nameX = x + mapWidth / 2 - nameW / 2;
        DrawText(map.displayName.c_str(), nameX + 1, y + mapHeight - 40 + 1, 24, Fade(BLACK, 0.7f));
        DrawText(map.displayName.c_str(), nameX, y + mapHeight - 40, 24, WHITE);

        // Map description (truncated)
        std::string desc = map.description;
        if (desc.length() > 40) desc = desc.substr(0, 37) + "...";
        int descW = MeasureText(desc.c_str(), 16);
        int descX = x + mapWidth / 2 - descW / 2;
        DrawText(desc.c_str(), descX, y + mapHeight - 15, 16, Fade(WHITE, 0.8f));

        // Selection indicator
        if (isSelected)
        {
            DrawRectangleLines(x - 3, y - 3, mapWidth + 6, mapHeight + 6, YELLOW);
            DrawRectangleLines(x - 6, y - 6, mapWidth + 12, mapHeight + 12, Fade(YELLOW, 0.5f));
        }
    }

    // Instructions
    const char* instructions = "[←/→] Navigate   [Enter] Select   [Esc] Back";
    int iw = MeasureText(instructions, 20);
    DrawText(instructions, GetScreenWidth() / 2 - iw / 2, GetScreenHeight() - 30, 20, GRAY);
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
        return selectedMap->name;
    }
    return "parkour_test"; // Default fallback
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
    DrawText("Console", 10, 10, 20, YELLOW);

    // Pause indicator
    DrawText("GAME PAUSED", GetScreenWidth() - 150, 10, 16, RED);

    // Output lines
    int startY = 40;
    size_t startLine = (m_consoleOutput.size() > (size_t)(consoleHeight - 80) / lineHeight) ?
                      m_consoleOutput.size() - (consoleHeight - 80) / lineHeight : 0;

    for (size_t i = startLine; i < m_consoleOutput.size(); ++i)
    {
        int y = startY + (i - startLine) * lineHeight;
        DrawText(m_consoleOutput[i].c_str(), 10, y, 16, WHITE);
    }

    // Input line
    int inputY = consoleHeight - 30;
    DrawText("]", 10, inputY, 16, GREEN);
    DrawText(m_consoleInput.c_str(), 25, inputY, 16, WHITE);

    // Blinking cursor
    if ((int)(GetTime() * 2) % 2 == 0)
    {
        int cursorX = 25 + MeasureText(m_consoleInput.c_str(), 16);
        DrawText("_", cursorX, inputY, 16, WHITE);
    }

    // Instructions
    const char* instructions = "[~] Toggle Console [↑/↓] History [Enter] Execute";
    int iw = MeasureText(instructions, 14);
    DrawText(instructions, GetScreenWidth() - iw - 10, GetScreenHeight() - 20, 14, GRAY);
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
        //unsigned int flags = GetWindowState();
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

    return "";
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
}

void Menu::SaveSettings()
{
    try
    {
        // Save current window settings
        m_config.SetResolution(GetScreenWidth(), GetScreenHeight());
        m_config.SetFullscreen(IsWindowFullscreen());
        m_config.SetVSync(IsWindowState(FLAG_VSYNC_HINT));

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
