#include "Menu.h"
#include "Engine/Engine.h"
#include <cstdio> // sscanf
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
    if (!m_currentMenu)
        return;
    if (m_state == MenuState::Credits || m_state == MenuState::Mods || m_state == MenuState::Video)
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
            SetWindowSize(w, h);
        }
        else if (opt.label == "Display Mode")
        {
            std::string mode = opt.values[opt.selectedIndex];
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
            if (opt.values[opt.selectedIndex] == "On")
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
            if (val == "Unlimited")
                SetTargetFPS(0);
            else
                SetTargetFPS(std::stoi(val));
        }
    }
}

void Menu::HandleMouseSelection()
{
    if (!m_currentMenu)
        return;
    if (m_state == MenuState::Credits || m_state == MenuState::Mods || m_state == MenuState::MapSelection)
        return;

    Vector2 mousePos = GetMousePosition();
    constexpr int kBtnW = 200, kBtnH = 50, kStartY = 300, kSpacing = 70;
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

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

void Menu::Render() const {

    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = (float)i / GetScreenHeight();
        DrawLine(0, i, GetScreenWidth(), i,
                 Color{(unsigned char)(15 + 25 * t), (unsigned char)(15 + 30 * t),
                       (unsigned char)(40 + 90 * t), 255});
    }
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));

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
    constexpr int kBtnW = 200, kBtnH = 50, kStartY = 300, kSpacing = 70;

    const char *title =
        (m_state == MenuState::Main) ? "Chained Decos" : (*m_currentMenu)[m_selected].label;
    int tw = MeasureText(title, 56);
    DrawText(title, GetScreenWidth() / 2 - tw / 2 + 4, 104, 56, Fade(BLACK, 0.75f));
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 100, 56, RAYWHITE);

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
        float targetScale = (hovered || selected) ? 1.10f : 1.0f;
        m_buttonScales[i] = Lerp(m_buttonScales[i], targetScale, 0.15f);

        int w = static_cast<int>(kBtnW * m_buttonScales[i]);
        int h = static_cast<int>(kBtnH * m_buttonScales[i]);
        int x = GetScreenWidth() / 2 - w / 2;
        int y = baseY - (h - kBtnH) / 2;
        Rectangle btnRect = {(float)x, (float)y, (float)w, (float)h};

        DrawRectangle(x + 5, y + 6, w, h, Fade(BLACK, 0.35f));

        Color topColor, bottomColor, borderColor;
        if (selected)
        {
            topColor = {255, 240, 200, 255};
            bottomColor = {220, 175, 90, 255};
            borderColor = ORANGE;
        }
        else if (hovered)
        {
            topColor = {245, 220, 165, 255};
            bottomColor = {205, 150, 85, 255};
            borderColor = {50, 35, 25, 255};
        }
        else
        {
            topColor = {200, 200, 200, 255};
            bottomColor = {130, 130, 130, 255};
            borderColor = {35, 35, 35, 255};
        }

        for (int j = 0; j < h; ++j)
        {
            float t = (float)j / h;
            Color c = {(unsigned char)(topColor.r + t * (bottomColor.r - topColor.r)),
                       (unsigned char)(topColor.g + t * (bottomColor.g - topColor.g)),
                       (unsigned char)(topColor.b + t * (bottomColor.b - topColor.b)), 255};
            DrawLine(x, y + j, x + w, y + j, c);
        }

        DrawRectangle(x + 1, y + 1, w, static_cast<int>(h * 0.4f), Fade(WHITE, 0.06f));

        if (selected)
        {
            DrawRectangleLinesEx(btnRect, 4, borderColor);
            DrawRectangleLinesEx(
                Rectangle{btnRect.x - 4, btnRect.y - 4, btnRect.width + 8, btnRect.height + 8}, 2,
                Fade(borderColor, 0.3f));
        }
        else
            DrawRectangleLinesEx(btnRect, 2, borderColor);

        int textW = MeasureText(item.label, 28);
        int textX = x + w / 2 - textW / 2;
        int textY = y + h / 2 - 28 / 2;
        DrawText(item.label, textX + 2, textY + 2, 28, Fade(BLACK, 0.7f));
        DrawText(item.label, textX, textY, 28, RAYWHITE);
    }

    const char *footer = "[Enter] Select   [Esc] Back   [↑/↓] Navigate   [Mouse] Click";
    int fw = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderSettingsMenu() const {
    int startY = 150, spacing = 70, fontSize = 28; // Increased spacing to prevent overlap
    DrawText("Video Settings", 80, 50, 40, ORANGE);

    for (size_t i = 0; i < m_videoOptions.size(); ++i)
    {
        auto &opt = m_videoOptions[i];
        int y = startY + static_cast<int>(i) * spacing;

        // Draw setting label
        DrawText(opt.label.c_str(), 80, y, fontSize,
                 (static_cast<int>(i) == m_selected) ? ORANGE : RAYWHITE);

        if (!opt.values.empty())
        {
            // Get current system value for display
            std::string currentValue = GetCurrentSettingValue(opt.label);

            // Show current value (smaller font, different position)
            if (!currentValue.empty())
            {
                int currentWidth = MeasureText(currentValue.c_str(), fontSize - 6);
                DrawText(currentValue.c_str(), 80 + 300, y + 5, fontSize - 6,
                         (static_cast<int>(i) == m_selected) ? Fade(GOLD, 0.8f) : Fade(YELLOW, 0.8f));
            }

            // Show selected value with arrows
            std::string displayValue;
            if (opt.selectedIndex < opt.values.size())
            {
                displayValue = "< " + opt.values[opt.selectedIndex] + " >";
            }

            if (!displayValue.empty())
            {
                int textWidth = MeasureText(displayValue.c_str(), fontSize);
                int xPos = GetScreenWidth() - textWidth - 80;
                DrawText(displayValue.c_str(), xPos, y, fontSize,
                         (static_cast<int>(i) == m_selected) ? GOLD : YELLOW);
            }
        }
    }

    std::string footer = "[Enter] Apply/Select [←/→] Change [↑/↓] Navigate [Esc] Back";
    int fw = MeasureText(footer.c_str(), 24);
    DrawText(footer.c_str(), GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderCredits()
{
    const char *title = "Credits";
    int tw = MeasureText(title, 48);
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 80, 48, ORANGE);

    int y = 160, fs = 26;
    DrawText("Developer: I#Oleg", 80, y, fs, RAYWHITE);
    y += 36;
    DrawText("Engine: raylib + rlImGui", 80, y, fs, RAYWHITE);
    y += 36;
    DrawText("UI: Custom styled buttons", 80, y, fs, RAYWHITE);

    const char *footer = "[Esc] Back";
    int fw2 = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw2 / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderMods()
{
    const char *title = "Mods";
    int tw = MeasureText(title, 48);
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 80, 48, ORANGE);

    int fs = 24;
    DrawText("No mods detected.", 80, 160, fs, RAYWHITE);
    DrawText("Place your mods in the 'resources/mods' folder.", 80, 200, fs, RAYWHITE);

    const char *footer = "[Esc] Back";
    int fw2 = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw2 / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderConfirmExit()
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));

    const char *msg = "Out of game?";
    int tw = MeasureText(msg, 48);
    DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 80, 48, ORANGE);

    const char *yes = "[Y/Enter] Yes";
    const char *no = "[N/Esc] No";
    int yw = MeasureText(yes, 32);
    int nw = MeasureText(no, 32);

    DrawText(yes, GetScreenWidth() / 2 - yw - 20, GetScreenHeight() / 2, 32, RAYWHITE);
    DrawText(no, GetScreenWidth() / 2 + 20, GetScreenHeight() / 2, 32, RAYWHITE);
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

    // Add default parkour test map
    m_availableMaps.push_back({
        "parkour_test",
        "Parkour Test Arena",
        "Classic parkour challenges with platforms, cubes, and spheres",
        "/resources/map_previews/parkour_test.png",
        ORANGE,
        true
    });

    // Add arena map
    m_availableMaps.push_back({
        "arena_test",
        "Doric Arena",
        "Ancient arena with challenging obstacles and precise jumps",
        "/resources/map_previews/arena_test.png",
        GOLD,
        true
    });

    // Add more maps as needed
    m_availableMaps.push_back({
        "training_ground",
        "Training Ground",
        "Beginner-friendly map for learning basic mechanics",
        "/resources/map_previews/training_ground.png",
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
                    AddConsoleOutput("VSync enabled");
                }
                else if (args == "off" || args == "0")
                {
                    ClearWindowState(FLAG_VSYNC_HINT);
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
